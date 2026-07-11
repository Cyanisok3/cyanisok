#include "../include/handlers/ChatSendHandler.h"

#include <chrono>
#include <future>
#include <iomanip>
#include <openssl/rand.h>
#include <sstream>

namespace
{

constexpr std::size_t kMaxQuestionBytes = 8000U;

std::string statusText(http::HttpResponse::HttpStatusCode statusCode)
{
    switch (statusCode)
    {
    case http::HttpResponse::k400BadRequest:
        return "Bad Request";
    case http::HttpResponse::k401Unauthorized:
        return "Unauthorized";
    case http::HttpResponse::k409Conflict:
        return "Conflict";
    case http::HttpResponse::k503ServiceUnavailable:
        return "Service Unavailable";
    case http::HttpResponse::k500InternalServerError:
        return "Internal Server Error";
    default:
        return "OK";
    }
}

void sendJsonAndClose(
    const muduo::net::TcpConnectionPtr& conn,
    const std::string& version,
    http::HttpResponse::HttpStatusCode statusCode,
    const json& body)
{
    const std::string responseBody = body.dump(4);
    const std::string httpVersion =
        version.empty() || version == "Unknown" ? "HTTP/1.1" : version;
    const std::string response =
        httpVersion + " " +
        std::to_string(static_cast<int>(statusCode)) + " " +
        statusText(statusCode) + "\r\n" +
        "Connection: close\r\n" +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + std::to_string(responseBody.size()) + "\r\n" +
        "Cache-Control: no-store\r\n\r\n" +
        responseBody;
    conn->send(response);
    conn->shutdown();
}

void sendSseHeaders(
    const muduo::net::TcpConnectionPtr& conn,
    const std::string& version)
{
    const std::string httpVersion =
        version.empty() || version == "Unknown" ? "HTTP/1.1" : version;
    conn->send(
        httpVersion + " 200 OK\r\n"
        "Content-Type: text/event-stream; charset=utf-8\r\n"
        "Cache-Control: no-cache, no-store, no-transform\r\n"
        "Connection: close\r\n"
        "X-Accel-Buffering: no\r\n\r\n");
}

std::string sseEvent(const std::string& eventName, const json& payload)
{
    return "event: " + eventName + "\r\n" +
           "data: " + payload.dump() + "\r\n\r\n";
}

void sendOnLoop(
    const std::weak_ptr<muduo::net::TcpConnection>& weakConn,
    const std::string& payload)
{
    const auto conn = weakConn.lock();
    if (!conn)
    {
        return;
    }
    conn->getLoop()->runInLoop([weakConn, payload] {
        const auto liveConn = weakConn.lock();
        if (liveConn && liveConn->connected())
        {
            liveConn->send(payload);
        }
    });
}

void shutdownOnLoop(
    const std::weak_ptr<muduo::net::TcpConnection>& weakConn)
{
    const auto conn = weakConn.lock();
    if (!conn)
    {
        return;
    }
    conn->getLoop()->runInLoop([weakConn] {
        const auto liveConn = weakConn.lock();
        if (liveConn && liveConn->connected())
        {
            liveConn->shutdown();
        }
    });
}

long long currentTimestampMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string generateRequestId()
{
    unsigned char bytes[12];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1)
    {
        return std::to_string(currentTimestampMs());
    }

    std::ostringstream id;
    id << std::hex << std::setfill('0');
    for (unsigned char byte : bytes)
    {
        id << std::setw(2) << static_cast<int>(byte);
    }
    return id.str();
}

} // namespace

void ChatSendHandler::handleStream(
    const muduo::net::TcpConnectionPtr& conn,
    const http::HttpRequest& req)
{
    int reservedUserId = -1;
    try
    {
        const auto session =
            server_->getSessionManager()->findSession(req);
        if (!session || session->getValue("isLoggedIn") != "true")
        {
            sendJsonAndClose(
                conn,
                req.getVersion(),
                http::HttpResponse::k401Unauthorized,
                {{"status", "error"}, {"message", "Unauthorized"}});
            return;
        }

        if (!server_->aiHelper_)
        {
            sendJsonAndClose(
                conn,
                req.getVersion(),
                http::HttpResponse::k503ServiceUnavailable,
                {{"status", "error"}, {"message", "AI service is not configured"}});
            return;
        }

        const int userId = std::stoi(session->getValue("userId"));
        const std::string username = session->getValue("username");
        std::string question;
        if (!req.getBody().empty())
        {
            const json parsed = json::parse(req.getBody());
            question = parsed.value("question", "");
        }
        if (question.empty() || question.size() > kMaxQuestionBytes)
        {
            sendJsonAndClose(
                conn,
                req.getVersion(),
                http::HttpResponse::k400BadRequest,
                {
                    {"status", "error"},
                    {"message", "Question must be 1-8000 UTF-8 bytes"}
                });
            return;
        }

        if (!server_->tryStartChat(userId))
        {
            sendJsonAndClose(
                conn,
                req.getVersion(),
                http::HttpResponse::k409Conflict,
                {
                    {"status", "error"},
                    {"message", "A chat request is already active"}
                });
            return;
        }
        reservedUserId = userId;

        const auto activeChatLease = std::shared_ptr<void>(
            nullptr,
            [server = server_, userId](void*) {
                server->finishChat(userId);
            });
        reservedUserId = -1;
        const std::weak_ptr<muduo::net::TcpConnection> weakConn = conn;
        const std::string requestId = generateRequestId();
        auto startPromise = std::make_shared<std::promise<void>>();
        const std::shared_future<void> startSignal =
            startPromise->get_future().share();

        const bool submitted = server_->aiExecutor_->trySubmit(
            [server = server_,
             weakConn,
             userId,
             username,
             question,
             requestId,
             activeChatLease,
             startSignal] {
                (void)activeChatLease;
                startSignal.wait();

                try
                {
                    server->chatRepository_.append(
                        userId,
                        username,
                        true,
                        question,
                        currentTimestampMs());
                    const auto context = server->chatRepository_.recent(
                        userId,
                        server->contextMessageLimit_);

                    const std::string answer = server->aiHelper_->chatStream(
                        context,
                        [weakConn, requestId](const std::string& token) {
                            sendOnLoop(
                                weakConn,
                                sseEvent(
                                    "delta",
                                    {
                                        {"request_id", requestId},
                                        {"content", token}
                                    }));
                        },
                        [weakConn] {
                            return weakConn.expired();
                        });

                    server->chatRepository_.append(
                        userId,
                        username,
                        false,
                        answer,
                        currentTimestampMs());
                    sendOnLoop(
                        weakConn,
                        sseEvent(
                            "done",
                            {
                                {"request_id", requestId},
                                {"content", answer}
                            }));
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Chat request " << requestId
                              << " failed: " << e.what();
                    sendOnLoop(
                        weakConn,
                        sseEvent(
                            "error",
                            {
                                {"request_id", requestId},
                                {"message", "Unable to complete chat request"}
                            }));
                }
                shutdownOnLoop(weakConn);
            });

        if (!submitted)
        {
            sendJsonAndClose(
                conn,
                req.getVersion(),
                http::HttpResponse::k503ServiceUnavailable,
                {
                    {"status", "error"},
                    {"message", "AI service is busy; try again shortly"}
                });
            return;
        }

        sendSseHeaders(conn, req.getVersion());
        startPromise->set_value();
    }
    catch (const std::exception& e)
    {
        if (reservedUserId >= 0)
        {
            server_->finishChat(reservedUserId);
        }
        LOG_WARN << "Invalid chat request: " << e.what();
        sendJsonAndClose(
            conn,
            req.getVersion(),
            http::HttpResponse::k400BadRequest,
            {{"status", "error"}, {"message", "Invalid chat request"}});
    }
}
