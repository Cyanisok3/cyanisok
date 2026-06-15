#include "../include/handlers/ChatLoginHandler.h"
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

void ChatLoginHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    auto contentType = req.getHeader("Content-Type");
    if (contentType.find("application/json") == std::string::npos || req.getBody().empty())
    {
        writeJsonResponse(req, resp, http::HttpResponse::k400BadRequest, "Bad Request",
            {{"status", "error"}, {"message", "JSON body required"}}, true);
        return;
    }

    try
    {
        json parsed = json::parse(req.getBody());
        std::string username = parsed.value("username", "");
        std::string password = parsed.value("password", "");
        if (!security::isValidUsername(username) || !security::isValidPassword(password))
        {
            writeJsonResponse(req, resp, http::HttpResponse::k400BadRequest, "Bad Request",
                {{"status", "error"}, {"message", "Invalid credentials format"}}, true);
            return;
        }

        const auto user = server_->userRepository_.findByUsername(username);
        if (user && security::verifyPassword(password, user->passwordHash))
        {
            auto session = server_->getSessionManager()->getSession(req, resp);

            session->setValue("userId", std::to_string(user->id));
            session->setValue("username", username);
            session->setValue("isLoggedIn", "true");

            writeJsonResponse(req, resp, http::HttpResponse::k200Ok, "OK",
                {{"success", true}, {"userId", user->id}, {"username", username}});
            return;
        }
        else
        {
            writeJsonResponse(req, resp, http::HttpResponse::k401Unauthorized, "Unauthorized",
                {{"status", "error"}, {"message", "Invalid username or password"}});
            return;
        }
    }
    catch (const json::exception&)
    {
        writeJsonResponse(req, resp, http::HttpResponse::k400BadRequest,
            "Bad Request",
            {{"status", "error"}, {"message", "Invalid JSON body"}}, true);
        return;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Login failed: " << e.what();
        writeJsonResponse(req, resp, http::HttpResponse::k500InternalServerError,
            "Internal Server Error",
            {{"status", "error"}, {"message", "Unable to process login"}}, true);
        return;
    }
}
