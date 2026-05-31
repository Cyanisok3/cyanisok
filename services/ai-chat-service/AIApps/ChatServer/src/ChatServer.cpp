#include "../include/handlers/ChatLoginHandler.h"
#include "../include/handlers/ChatRegisterHandler.h"
#include "../include/handlers/ChatLogoutHandler.h"
#include "../include/handlers/ChatSendHandler.h"
#include "../include/handlers/AIUploadSendHandler.h"
#include "../include/handlers/ChatHistoryHandler.h"

#include "../include/handlers/HealthHandler.h"
#include "../include/ChatServer.h"
#include "../../../HttpServer/include/http/HttpRequest.h"
#include "../../../HttpServer/include/http/HttpResponse.h"
#include "../../../HttpServer/include/http/HttpServer.h"

using namespace http;

ChatServer::ChatServer(int port,
    const std::string& name,
    muduo::net::TcpServer::Option option)
    : httpServer_(port, name, option)
{
    initialize();
}

void ChatServer::initialize() {
    std::cout << "ChatServer initialize start  ! " << std::endl;

    const char* mysqlHost = std::getenv("MYSQL_HOST");
    const char* mysqlUser = std::getenv("MYSQL_USER");
    const char* mysqlPass = std::getenv("MYSQL_PASSWORD");
    const char* mysqlDb = std::getenv("MYSQL_DATABASE");

    std::string host = mysqlHost ? mysqlHost : "mysql";
    std::string user = mysqlUser ? mysqlUser : "root";
    std::string pass = mysqlPass ? mysqlPass : "123456";
    std::string db = mysqlDb ? mysqlDb : "chatserver";

    http::MysqlUtil::init("tcp://" + host + ":3306", user, pass, db, 5);
    initializeSession();
    initializeMiddleware();
    initializeRouter();
}

void ChatServer::initChatMessage() {
    std::cout << "initChatMessage start ! " << std::endl;
    readDataFromMySQL();
    std::cout << "initChatMessage success ! " << std::endl;
}

void ChatServer::readDataFromMySQL() {
    const char* apiKey = std::getenv("DASHSCOPE_API_KEY");
    if (!apiKey) {
        std::cerr << "Error: DASHSCOPE_API_KEY not found in environment!" << std::endl;
        return;
    }

    std::string sql = "SELECT user_id, username, is_user, content, ts FROM chat_message ORDER BY user_id ASC, ts ASC, message_id ASC";

    sql::ResultSet* res;
    try {
        res = mysqlUtil_.executeQuery(sql);
    }
    catch (const std::exception& e) {
        std::cerr << "MySQL query failed: " << e.what() << std::endl;
        return;
    }

    while (res->next()) {
        long long user_id = 0;
        std::string username, content;
        long long ts = 0;
        int is_user = 1;

        try {
            user_id = res->getInt64("user_id");
            username = res->getString("username");
            content = res->getString("content");
            ts = res->getInt64("ts");
            is_user = res->getInt("is_user");
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to read row: " << e.what() << std::endl;
            continue;
        }

        std::shared_ptr<AIHelper> helper;
        auto it = chatInformation.find(user_id);
        if (it == chatInformation.end()) {
            helper = std::make_shared<AIHelper>(apiKey);
            chatInformation[user_id] = helper;
        }
        else {
            helper = it->second;
        }

        helper->restoreMessage(is_user != 0, content, ts);
    }

    std::cout << "readDataFromMySQL finished" << std::endl;
}

void ChatServer::setThreadNum(int numThreads) {
    httpServer_.setThreadNum(numThreads);
}

void ChatServer::start() {
    httpServer_.start();
}

void ChatServer::initializeRouter() {
    httpServer_.Post("/login", std::make_shared<ChatLoginHandler>(this));
    httpServer_.Post("/register", std::make_shared<ChatRegisterHandler>(this));
    httpServer_.Post("/user/logout", std::make_shared<ChatLogoutHandler>(this));
    auto chatSendHandler = std::make_shared<ChatSendHandler>(this);
    httpServer_.PostStream("/chat/send", [chatSendHandler](const muduo::net::TcpConnectionPtr& conn, const http::HttpRequest& req) {
        chatSendHandler->handleStream(conn, req);
    });
    httpServer_.Post("/upload/send", std::make_shared<AIUploadSendHandler>(this));
    httpServer_.Post("/chat/history", std::make_shared<ChatHistoryHandler>(this));
    httpServer_.Get("/health", std::make_shared<HealthHandler>());
}

void ChatServer::initializeSession() {
    auto sessionStorage = std::make_unique<http::session::MemorySessionStorage>();
    auto sessionManager = std::make_unique<http::session::SessionManager>(std::move(sessionStorage));
    setSessionManager(std::move(sessionManager));
}

void ChatServer::initializeMiddleware() {
    auto corsMiddleware = std::make_shared<http::middleware::CorsMiddleware>();
    httpServer_.addMiddleware(corsMiddleware);
}

void ChatServer::packageResp(const std::string& version,
    http::HttpResponse::HttpStatusCode statusCode,
    const std::string& statusMsg,
    bool close,
    const std::string& contentType,
    int contentLen,
    const std::string& body,
    http::HttpResponse* resp)
{
    if (resp == nullptr)
    {
        LOG_ERROR << "Response pointer is null";
        return;
    }

    try
    {
        resp->setVersion(version);
        resp->setStatusCode(statusCode);
        resp->setStatusMessage(statusMsg);
        resp->setCloseConnection(close);
        resp->setContentType(contentType);
        resp->setContentLength(contentLen);
        resp->setBody(body);

        LOG_INFO << "Response packaged successfully";
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Error in packageResp: " << e.what();
        resp->setStatusCode(http::HttpResponse::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setCloseConnection(true);
    }
}
