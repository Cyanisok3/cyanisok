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

        int userId = insertUser(username, password);

        if (userId != -1)
        {
            writeJsonResponse(req, resp, http::HttpResponse::k200Ok, "OK",
                {{"status", "success"}, {"message", "Register successful"}, {"userId", userId}});
        }
        else
        {
            writeJsonResponse(req, resp, http::HttpResponse::k409Conflict, "Conflict",
                {{"status", "error"}, {"message", "Username already exists"}});
        }
    }
    catch (const std::exception& e) {
        writeJsonResponse(req, resp, http::HttpResponse::k500InternalServerError, "Internal Server Error",
            {{"status", "error"}, {"message", e.what()}}, true);
    }
}

int ChatRegisterHandler::insertUser(const std::string& username, const std::string& password)
{
    if (!isUserExist(username))
    {
        std::string passwordHash = security::hashPassword(password);
        std::string sql = "INSERT INTO users (username, password_hash) VALUES (?, ?)";
        mysqlUtil_.executeUpdate(sql, username, passwordHash);

        std::string sql2 = "SELECT id FROM users WHERE username = ?";
        auto res = mysqlUtil_.executeQuery(sql2, username);
        if (res->next())
        {
            return res->getInt("id");
        }
    }
    return -1;
}

bool ChatRegisterHandler::isUserExist(const std::string& username)
{
    std::string sql = "SELECT id FROM users WHERE username = ?";
    auto res = mysqlUtil_.executeQuery(sql, username);
    if (res->next())
    {
        return true;
    }
    return false;
}
