#pragma once

#include <muduo/net/TcpConnection.h>

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../ChatServer.h"

class ChatSendHandler : public http::router::RouterHandler
{
public:
    explicit ChatSendHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
    void handleStream(const muduo::net::TcpConnectionPtr& conn, const http::HttpRequest& req);
private:
    ChatServer* server_;
    http::MysqlUtil     mysqlUtil_;
};
