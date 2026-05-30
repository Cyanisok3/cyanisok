#include "../include/handlers/ChatSendHandler.h"

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
