#pragma once

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../ChatServer.h"


class ChatRegisterHandler : public http::router::RouterHandler
{
public:
    explicit ChatRegisterHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:
    ChatServer* server_;
};
