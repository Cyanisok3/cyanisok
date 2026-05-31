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

        int userId = queryUserId(username, password);
        if (userId != -1)
        {
            auto session = server_->getSessionManager()->getSession(req, resp);

            session->setValue("userId", std::to_string(userId));
            session->setValue("username", username);
            session->setValue("isLoggedIn", "true");

            {
                std::lock_guard<std::mutex> lock(server_->mutexForOnlineUsers_);
                server_->onlineUsers_[userId] = true;
            }

            writeJsonResponse(req, resp, http::HttpResponse::k200Ok, "OK",
                {{"success", true}, {"userId", userId}, {"username", username}});
            return;
        }
        else
        {
            writeJsonResponse(req, resp, http::HttpResponse::k401Unauthorized, "Unauthorized",
                {{"status", "error"}, {"message", "Invalid username or password"}});
            return;
        }
    }
    catch (const std::exception& e)
    {
        writeJsonResponse(req, resp, http::HttpResponse::k400BadRequest, "Bad Request",
            {{"status", "error"}, {"message", e.what()}}, true);
        return;
    }
}

int ChatLoginHandler::queryUserId(const std::string& username, const std::string& password)
{
    std::string sql = "SELECT id, password_hash FROM users WHERE username = ?";
    auto res = mysqlUtil_.executeQuery(sql, username);
    if (res->next())
    {
        int id = res->getInt("id");
        std::string passwordHash = res->getString("password_hash");
        if (security::verifyPassword(password, passwordHash))
        {
            return id;
        }
    }
    return -1;
}
