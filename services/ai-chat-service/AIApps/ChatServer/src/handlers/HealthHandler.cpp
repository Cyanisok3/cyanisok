#include "../include/handlers/HealthHandler.h"
#include "../include/ChatServer.h"
#include "../../../../HttpServer/include/utils/JsonUtil.h"

void HealthHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
    resp->setCloseConnection(false);
    resp->setContentType("application/json");
    std::string body = "{\"status\":\"ok\"}";
    resp->setContentLength(body.size());
    resp->setBody(body);
}

void ReadyHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    const bool databaseReady = server_->chatRepository_.ping();
    const bool aiReady = server_->aiConfigured();
    const bool ready = databaseReady && aiReady;

    json bodyJson = {
        {"status", ready ? "ready" : "not_ready"},
        {"database", databaseReady},
        {"ai_configured", aiReady},
        {"image_recognition", server_->imageRecognizerAvailable()}
    };
    const std::string body = bodyJson.dump();
    resp->setStatusLine(
        req.getVersion(),
        ready ? http::HttpResponse::k200Ok
              : http::HttpResponse::k503ServiceUnavailable,
        ready ? "OK" : "Service Unavailable");
    resp->setCloseConnection(false);
    resp->setContentType("application/json");
    resp->setContentLength(body.size());
    resp->setBody(body);
}
