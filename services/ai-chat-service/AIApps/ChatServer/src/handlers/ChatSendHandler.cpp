#include "../include/handlers/ChatSendHandler.h"

#include <thread>

namespace
{

std::string statusText(http::HttpResponse::HttpStatusCode statusCode)
{
    switch (statusCode)
    {
    case http::HttpResponse::k400BadRequest:
        return "Bad Request";
    case http::HttpResponse::k401Unauthorized:
        return "Unauthorized";
    case http::HttpResponse::k500InternalServerError:
        return "Internal Server Error";
    default:
        return "OK";
    }
}

void sendJsonAndClose(const muduo::net::TcpConnectionPtr& conn,
                      const std::string& version,
                      http::HttpResponse::HttpStatusCode statusCode,
                      const json& body)
{
    std::string responseBody = body.dump(4);
    std::string httpVersion = version.empty() || version == "Unknown" ? "HTTP/1.1" : version;
    std::string response =
        httpVersion + " " + std::to_string(static_cast<int>(statusCode)) + " " + statusText(statusCode) + "\r\n" +
        "Connection: close\r\n" +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + std::to_string(responseBody.size()) + "\r\n" +
        "\r\n" +
        responseBody;
    conn->send(response);
    conn->shutdown();
}

void sendSseHeaders(const muduo::net::TcpConnectionPtr& conn, const std::string& version)
{
    std::string httpVersion = version.empty() || version == "Unknown" ? "HTTP/1.1" : version;
    std::string headers =
        httpVersion + " 200 OK\r\n"
        "Content-Type: text/event-stream; charset=utf-8\r\n"
        "Cache-Control: no-cache, no-transform\r\n"
        "Connection: close\r\n"
        "X-Accel-Buffering: no\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n";
    conn->send(headers);
}

std::string sseEvent(const std::string& eventName, const json& payload)
{
    return "event: " + eventName + "\r\n" +
           "data: " + payload.dump() + "\r\n\r\n";
}

void sendOnLoop(const muduo::net::TcpConnectionPtr& conn, const std::string& payload)
{
    auto loop = conn->getLoop();
    loop->runInLoop([conn, payload]() {
        if (conn->connected())
        {
            conn->send(payload);
        }
    });
}

void shutdownOnLoop(const muduo::net::TcpConnectionPtr& conn)
{
    auto loop = conn->getLoop();
    loop->runInLoop([conn]() {
        if (conn->connected())
        {
            conn->shutdown();
        }
    });
}

} // namespace

void ChatSendHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try
    {
        auto session = server_->getSessionManager()->getSession(req, resp);
        LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
        if (session->getValue("isLoggedIn") != "true")
        {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                "Unauthorized", true, "application/json", errorBody.size(),
                errorBody, resp);
            return;
        }

        int userId = std::stoi(session->getValue("userId"));
        std::string username = session->getValue("username");

        std::shared_ptr<AIHelper> AIHelperPtr;
        {
            std::lock_guard<std::mutex> lock(server_->mutexForChatInformation);
            if (server_->chatInformation.find(userId) == server_->chatInformation.end()) {
                const char* apiKey = std::getenv("DASHSCOPE_API_KEY");
                if (!apiKey) {
                    json errorResp;
                    errorResp["status"] = "error";
                    errorResp["message"] = "AI service is not configured";
                    std::string errorBody = errorResp.dump(4);
                    server_->packageResp(req.getVersion(), http::HttpResponse::k500InternalServerError,
                        "Internal Server Error", true, "application/json", errorBody.size(),
                        errorBody, resp);
                    return;
                }
                server_->chatInformation.emplace(
                    userId,
                    std::make_shared<AIHelper>(apiKey)
                );
            }
            AIHelperPtr = server_->chatInformation[userId];
        }

        std::string userQuestion;
        auto body = req.getBody();
        if (!body.empty()) {
            auto j = json::parse(body);
            if (j.contains("question")) userQuestion = j["question"];
        }
        if (userQuestion.empty() || userQuestion.size() > 8000) {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Question must be 1-8000 characters";
            std::string errorBody = errorResp.dump(4);
            server_->packageResp(req.getVersion(), http::HttpResponse::k400BadRequest,
                "Bad Request", true, "application/json", errorBody.size(),
                errorBody, resp);
            return;
        }

        AIHelperPtr->addMessage(userId, username, true, userQuestion);

        std::string aiInformation = AIHelperPtr->chat(userId, username);
        json successResp;
        successResp["success"] = true;
        successResp["information"] = aiInformation;
        std::string successBody = successResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
        return;
    }
    catch (const std::exception& e)
    {
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}

void ChatSendHandler::handleStream(const muduo::net::TcpConnectionPtr& conn, const http::HttpRequest& req)
{
    try
    {
        http::HttpResponse sessionResponse(false);
        auto session = server_->getSessionManager()->getSession(req, &sessionResponse);
        LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
        if (session->getValue("isLoggedIn") != "true")
        {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            sendJsonAndClose(conn, req.getVersion(), http::HttpResponse::k401Unauthorized, errorResp);
            return;
        }

        int userId = std::stoi(session->getValue("userId"));
        std::string username = session->getValue("username");

        std::shared_ptr<AIHelper> AIHelperPtr;
        {
            std::lock_guard<std::mutex> lock(server_->mutexForChatInformation);
            if (server_->chatInformation.find(userId) == server_->chatInformation.end()) {
                const char* apiKey = std::getenv("DASHSCOPE_API_KEY");
                if (!apiKey) {
                    json errorResp;
                    errorResp["status"] = "error";
                    errorResp["message"] = "AI service is not configured";
                    sendJsonAndClose(conn, req.getVersion(), http::HttpResponse::k500InternalServerError, errorResp);
                    return;
                }
                server_->chatInformation.emplace(
                    userId,
                    std::make_shared<AIHelper>(apiKey)
                );
            }
            AIHelperPtr = server_->chatInformation[userId];
        }

        std::string userQuestion;
        auto body = req.getBody();
        if (!body.empty()) {
            auto j = json::parse(body);
            if (j.contains("question")) userQuestion = j["question"];
        }
        if (userQuestion.empty() || userQuestion.size() > 8000) {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Question must be 1-8000 characters";
            sendJsonAndClose(conn, req.getVersion(), http::HttpResponse::k400BadRequest, errorResp);
            return;
        }

        AIHelperPtr->addMessage(userId, username, true, userQuestion);
        sendSseHeaders(conn, req.getVersion());

        std::thread([conn, AIHelperPtr, userId, username]() {
            try
            {
                std::string fullAnswer = AIHelperPtr->chatStream(
                    userId,
                    username,
                    [conn](const std::string& token) {
                        json payload;
                        payload["content"] = token;
                        sendOnLoop(conn, sseEvent("delta", payload));
                    });

                json donePayload;
                donePayload["content"] = fullAnswer;
                sendOnLoop(conn, sseEvent("done", donePayload));
            }
            catch (const std::exception& e)
            {
                json errorPayload;
                errorPayload["message"] = e.what();
                sendOnLoop(conn, sseEvent("error", errorPayload));
            }

            shutdownOnLoop(conn);
        }).detach();
    }
    catch (const std::exception& e)
    {
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        sendJsonAndClose(conn, req.getVersion(), http::HttpResponse::k400BadRequest, failureResp);
    }
}
