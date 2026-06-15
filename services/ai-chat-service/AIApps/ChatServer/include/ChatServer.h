#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include "../../../HttpServer/include/http/HttpServer.h"
#include "AIUtil/AIHelper.h"
#include "AIUtil/BoundedExecutor.h"
#include "AIUtil/ImageRecognizer.h"
#include "repositories/ChatRepository.h"
#include "repositories/UserRepository.h"

class ChatLoginHandler;
class ChatRegisterHandler;
class ChatLogoutHandler;
class ChatSendHandler;
class ChatHistoryHandler;
class AIUploadSendHandler;
class ReadyHandler;

class ChatServer
{
public:
    ChatServer(
        int port,
        const std::string& name,
        muduo::net::TcpServer::Option option =
            muduo::net::TcpServer::kNoReusePort);
    ~ChatServer();

    void setThreadNum(int numThreads);
    void start();

private:
    friend class ChatLoginHandler;
    friend class ChatRegisterHandler;
    friend class ChatLogoutHandler;
    friend class ChatSendHandler;
    friend class AIUploadSendHandler;
    friend class ChatHistoryHandler;
    friend class ReadyHandler;

    void initialize();
    void initializeSession();
    void initializeRouter();
    void initializeOptionalImageRecognizer();
    void startRetentionWorker();
    void stopBackgroundWorkers();
    bool tryStartChat(int userId);
    void finishChat(int userId);
    bool aiConfigured() const;
    bool imageRecognizerAvailable() const;

    void packageResp(
        const std::string& version,
        http::HttpResponse::HttpStatusCode statusCode,
        const std::string& statusMsg,
        bool close,
        const std::string& contentType,
        int contentLen,
        const std::string& body,
        http::HttpResponse* resp);

    void setSessionManager(
        std::unique_ptr<http::session::SessionManager> manager)
    {
        httpServer_.setSessionManager(std::move(manager));
    }

    http::session::SessionManager* getSessionManager() const
    {
        return httpServer_.getSessionManager();
    }

    http::HttpServer httpServer_;
    UserRepository userRepository_;
    ChatRepository chatRepository_;
    std::unique_ptr<AIHelper> aiHelper_;
    std::unique_ptr<BoundedExecutor> aiExecutor_;
    std::unique_ptr<ImageRecognizer> imageRecognizer_;
    mutable std::mutex imageRecognizerMutex_;

    std::mutex activeChatsMutex_;
    std::unordered_set<int> activeChats_;

    std::atomic<bool> stopRetention_{false};
    std::thread retentionThread_;
    std::mutex retentionMutex_;
    std::condition_variable retentionWakeup_;

    int contextMessageLimit_ = 40;
    int historyMessageLimit_ = 200;
    int retentionDays_ = 90;
};
