#pragma once

#include "../ChatServer.h"

class AIUploadSendHandler
{
public:
    explicit AIUploadSendHandler(ChatServer* server) : server_(server) {}

    void handleStream(
        const muduo::net::TcpConnectionPtr& conn,
        const http::HttpRequest& req);

private:
    ChatServer* server_;
};
