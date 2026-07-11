#pragma once

#include <muduo/net/TcpConnection.h>

#include "../ChatServer.h"

class ChatSendHandler
{
public:
    explicit ChatSendHandler(ChatServer* server) : server_(server) {}

    void handleStream(const muduo::net::TcpConnectionPtr& conn, const http::HttpRequest& req);
private:
    ChatServer* server_;
};
