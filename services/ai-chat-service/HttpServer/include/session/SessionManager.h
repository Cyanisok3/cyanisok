#pragma once

#include "SessionStorage.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include <atomic>
#include <chrono>
#include <memory>

namespace http
{
namespace session
{

class SessionManager
{
public:
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);

    std::shared_ptr<Session> getSession(const HttpRequest& req, HttpResponse* resp);
    std::shared_ptr<Session> findSession(const HttpRequest& req);
    std::shared_ptr<Session> rotateSession(
        const HttpRequest& req,
        HttpResponse* resp);

    void destroySession(const std::string& sessionId);
    void clearSessionCookie(HttpResponse* resp);

    void cleanExpiredSessions();

    void updateSession(std::shared_ptr<Session> session)
    {
        storage_->save(session);
    }
private:
    std::shared_ptr<Session> createSession(HttpResponse* resp);
    std::string generateSessionId();
    std::string getSessionIdFromCookie(const HttpRequest& req);
    void setSessionCookie(const std::string& sessionId, HttpResponse* resp);
    void cleanExpiredSessionsIfDue();
    std::string cookieAttributes() const;
    int sessionMaxAge() const;

private:
    std::unique_ptr<SessionStorage> storage_;
    std::atomic<long long> nextCleanupAtMs_{0};
};

} // namespace session
} // namespace http
