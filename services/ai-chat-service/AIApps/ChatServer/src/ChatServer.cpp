#include "../include/ChatServer.h"

#include "../include/handlers/AIUploadSendHandler.h"
#include "../include/handlers/ChatHistoryHandler.h"
#include "../include/handlers/ChatLoginHandler.h"
#include "../include/handlers/ChatLogoutHandler.h"
#include "../include/handlers/ChatRegisterHandler.h"
#include "../include/handlers/ChatSendHandler.h"
#include "../include/handlers/HealthHandler.h"

#include <chrono>
#include <cstdlib>
#include <iostream>

#include "../../../HttpServer/include/http/HttpRequest.h"
#include "../../../HttpServer/include/http/HttpResponse.h"
#include "../../../HttpServer/include/session/SessionManager.h"
#include "../../../HttpServer/include/session/SessionStorage.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"

using namespace http;

namespace
{

int readPositiveInt(const char* key, int fallback)
{
    const char* raw = std::getenv(key);
    if (!raw || raw[0] == '\0')
    {
        return fallback;
    }
    try
    {
        const int value = std::stoi(raw);
        return value > 0 ? value : fallback;
    }
    catch (...)
    {
        return fallback;
    }
}

bool readBool(const char* key, bool fallback)
{
    const char* raw = std::getenv(key);
    if (!raw)
    {
        return fallback;
    }
    const std::string value(raw);
    return value == "1" || value == "true" || value == "TRUE";
}

std::string envOrDefault(const char* key, const std::string& fallback)
{
    const char* raw = std::getenv(key);
    return raw && raw[0] != '\0' ? raw : fallback;
}

} // namespace

ChatServer::ChatServer(
    int port,
    const std::string& name,
    muduo::net::TcpServer::Option option)
    : httpServer_(port, name, option)
{
    initialize();
}

ChatServer::~ChatServer()
{
    stopBackgroundWorkers();
}

void ChatServer::initialize()
{
    const std::string host = envOrDefault("MYSQL_HOST", "mysql");
    const std::string port = envOrDefault("MYSQL_PORT", "3306");
    const std::string user = envOrDefault("MYSQL_USER", "chatuser");
    const std::string password = envOrDefault("MYSQL_PASSWORD", "");
    const std::string database = envOrDefault("MYSQL_DATABASE", "chatserver");
    if (password.empty())
    {
        throw std::runtime_error("MYSQL_PASSWORD must be configured");
    }

    http::MysqlUtil::init(
        "tcp://" + host + ":" + port,
        user,
        password,
        database,
        static_cast<size_t>(readPositiveInt("MYSQL_POOL_SIZE", 5)));

    contextMessageLimit_ = readPositiveInt("CHAT_CONTEXT_MESSAGES", 40);
    historyMessageLimit_ = readPositiveInt("CHAT_HISTORY_MESSAGES", 200);
    retentionDays_ = readPositiveInt("CHAT_RETENTION_DAYS", 90);

    aiExecutor_ = std::make_unique<BoundedExecutor>(
        static_cast<size_t>(readPositiveInt("AI_WORKER_COUNT", 4)),
        static_cast<size_t>(readPositiveInt("AI_QUEUE_CAPACITY", 32)));

    const char* apiKey = std::getenv("DEEPSEEK_API_KEY");
    if (!apiKey || apiKey[0] == '\0')
    {
        apiKey = std::getenv("DASHSCOPE_API_KEY");
        if (apiKey && apiKey[0] != '\0')
        {
            LOG_WARN << "DASHSCOPE_API_KEY is deprecated; use DEEPSEEK_API_KEY";
        }
    }

    if (apiKey && apiKey[0] != '\0')
    {
        aiHelper_ = std::make_unique<AIHelper>(apiKey);
    }
    else
    {
        LOG_ERROR << "DEEPSEEK_API_KEY is not configured";
    }

    initializeOptionalImageRecognizer();
    initializeSession();
    initializeRouter();
    startRetentionWorker();
}

void ChatServer::initializeOptionalImageRecognizer()
{
    if (!readBool("IMAGE_RECOGNITION_ENABLED", true))
    {
        LOG_WARN << "Image recognition is disabled";
        return;
    }

    try
    {
        imageRecognizer_ = std::make_unique<ImageRecognizer>(
            envOrDefault(
                "IMAGE_MODEL_PATH",
                "/root/models/mobilenetv2/mobilenetv2-7.onnx"),
            envOrDefault(
                "IMAGE_LABEL_PATH",
                "/root/imagenet_classes.txt"));
        imageExecutor_ = std::make_unique<BoundedExecutor>(
            1,
            static_cast<size_t>(readPositiveInt("IMAGE_QUEUE_CAPACITY", 4)));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Image recognition unavailable: " << e.what();
        imageRecognizer_.reset();
    }
}

void ChatServer::initializeSession()
{
    auto storage = std::make_unique<http::session::MemorySessionStorage>();
    setSessionManager(std::make_unique<http::session::SessionManager>(
        std::move(storage)));
}

void ChatServer::initializeRouter()
{
    httpServer_.Post("/login", std::make_shared<ChatLoginHandler>(this));
    if (readBool("REGISTRATION_ENABLED", false))
    {
        httpServer_.Post(
            "/register",
            std::make_shared<ChatRegisterHandler>(this));
    }
    else
    {
        LOG_WARN << "Public registration is disabled";
    }
    httpServer_.Post("/user/logout", std::make_shared<ChatLogoutHandler>(this));

    auto chatSendHandler = std::make_shared<ChatSendHandler>(this);
    httpServer_.PostStream(
        "/chat/send",
        [chatSendHandler](
            const muduo::net::TcpConnectionPtr& conn,
            const http::HttpRequest& req) {
            chatSendHandler->handleStream(conn, req);
        });

    auto uploadSendHandler = std::make_shared<AIUploadSendHandler>(this);
    httpServer_.PostStream(
        "/upload/send",
        [uploadSendHandler](
            const muduo::net::TcpConnectionPtr& conn,
            const http::HttpRequest& req) {
            uploadSendHandler->handleStream(conn, req);
        });
    httpServer_.Post(
        "/chat/history",
        std::make_shared<ChatHistoryHandler>(this));
    httpServer_.Get("/health", std::make_shared<HealthHandler>());
    httpServer_.Get("/ready", std::make_shared<ReadyHandler>(this));
}

void ChatServer::startRetentionWorker()
{
    retentionThread_ = std::thread([this] {
        while (!stopRetention_.load())
        {
            try
            {
                const auto now = std::chrono::system_clock::now();
                const auto cutoff = now - std::chrono::hours(24 * retentionDays_);
                const long long cutoffMs =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        cutoff.time_since_epoch()).count();

                while (!stopRetention_.load() &&
                       chatRepository_.deleteOlderThan(cutoffMs, 500) == 500)
                {
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "Chat retention cleanup failed: " << e.what();
            }

            std::unique_lock<std::mutex> lock(retentionMutex_);
            retentionWakeup_.wait_for(
                lock,
                std::chrono::hours(24),
                [this] { return stopRetention_.load(); });
        }
    });
}

void ChatServer::stopBackgroundWorkers()
{
    stopRetention_.store(true);
    retentionWakeup_.notify_all();
    if (retentionThread_.joinable())
    {
        retentionThread_.join();
    }
    if (aiExecutor_)
    {
        aiExecutor_->shutdown();
    }
    if (imageExecutor_)
    {
        imageExecutor_->shutdown();
    }
}

bool ChatServer::tryStartChat(int userId)
{
    std::lock_guard<std::mutex> lock(activeChatsMutex_);
    return activeChats_.insert(userId).second;
}

void ChatServer::finishChat(int userId)
{
    std::lock_guard<std::mutex> lock(activeChatsMutex_);
    activeChats_.erase(userId);
}

bool ChatServer::aiConfigured() const
{
    return aiHelper_ != nullptr;
}

bool ChatServer::imageRecognizerAvailable() const
{
    return imageRecognizer_ != nullptr;
}

void ChatServer::setThreadNum(int numThreads)
{
    httpServer_.setThreadNum(numThreads);
}

void ChatServer::start()
{
    httpServer_.start();
}

void ChatServer::packageResp(
    const std::string& version,
    http::HttpResponse::HttpStatusCode statusCode,
    const std::string& statusMsg,
    bool close,
    const std::string& contentType,
    int contentLen,
    const std::string& body,
    http::HttpResponse* resp)
{
    if (!resp)
    {
        return;
    }
    resp->setVersion(version);
    resp->setStatusCode(statusCode);
    resp->setStatusMessage(statusMsg);
    resp->setCloseConnection(close);
    resp->setContentType(contentType);
    resp->setContentLength(contentLen);
    resp->setBody(body);
}
