#pragma once

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../ChatServer.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"

class ChatLoginHandler : public http::router::RouterHandler
{
public:
    explicit ChatLoginHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    ChatServer* server_;
};
