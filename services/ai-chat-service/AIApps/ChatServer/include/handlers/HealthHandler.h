#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"

class ChatServer;

class HealthHandler : public http::router::RouterHandler
{
public:
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
};

class ReadyHandler : public http::router::RouterHandler
{
public:
    explicit ReadyHandler(ChatServer* server) : server_(server) {}
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    ChatServer* server_;
};
