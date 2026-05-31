#include "../include/handlers/HealthHandler.h"

void HealthHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
    resp->setCloseConnection(false);
    resp->setContentType("application/json");
    std::string body = "{\"status\":\"ok\"}";
    resp->setContentLength(body.size());
    resp->setBody(body);
}
