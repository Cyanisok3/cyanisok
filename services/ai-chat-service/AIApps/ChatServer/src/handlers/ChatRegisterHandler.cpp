#include "../include/handlers/ChatRegisterHandler.h"
#include "../include/utils/SecurityUtil.h"

namespace
{

void writeJsonResponse(const http::HttpRequest& req,
                       http::HttpResponse* resp,
                       http::HttpResponse::HttpStatusCode status,
                       const std::string& message,
                       const json& body,
                       bool close = false)
{
    std::string responseBody = body.dump(4);
    resp->setStatusLine(req.getVersion(), status, message);
    resp->setCloseConnection(close);
    resp->setContentType("application/json");
    resp->setContentLength(responseBody.size());
    resp->setBody(responseBody);
}

} // namespace

void ChatRegisterHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try {
        json parsed = json::parse(req.getBody());
        std::string username = parsed.value("username", "");
        std::string password = parsed.value("password", "");

        if (!security::isValidUsername(username) || !security::isValidPassword(password))
        {
            writeJsonResponse(req, resp, http::HttpResponse::k400BadRequest, "Bad Request",
                {{"status", "error"}, {"message", "Username must be 3-32 letters, numbers, underscores, or hyphens; password must be 8-128 characters."}}, true);
            return;
        }

        const std::string passwordHash = security::hashPassword(password);
        const auto userId = server_->userRepository_.create(username, passwordHash);

        if (userId)
        {
            writeJsonResponse(req, resp, http::HttpResponse::k200Ok, "OK",
                {{"status", "success"}, {"message", "Register successful"}, {"userId", *userId}});
        }
        else
        {
            writeJsonResponse(req, resp, http::HttpResponse::k409Conflict, "Conflict",
                {{"status", "error"}, {"message", "Username already exists"}});
        }
    }
    catch (const json::exception&) {
        writeJsonResponse(req, resp, http::HttpResponse::k400BadRequest,
            "Bad Request",
            {{"status", "error"}, {"message", "Invalid JSON body"}}, true);
    }
    catch (const std::exception& e) {
        LOG_ERROR << "Registration failed: " << e.what();
        writeJsonResponse(req, resp, http::HttpResponse::k500InternalServerError, "Internal Server Error",
            {{"status", "error"}, {"message", "Unable to create account"}}, true);
    }
}
