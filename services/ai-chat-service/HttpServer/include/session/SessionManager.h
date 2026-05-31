#pragma once

#include "SessionStorage.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include <memory>
#include <random>

namespace http
{
namespace session
{

class SessionManager
{
public:
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);

    std::shared_ptr<Session> getSession(const HttpRequest& req, HttpResponse* resp);

    void destroySession(const std::string& sessionId);
    void clearSessionCookie(HttpResponse* resp);

    void cleanExpiredSessions();

    void updateSession(std::shared_ptr<Session> session)
    {
        storage_->save(session);
    }
private:
    std::string generateSessionId();
    std::string getSessionIdFromCookie(const HttpRequest& req);
    void setSessionCookie(const std::string& sessionId, HttpResponse* resp);
    std::string cookieAttributes() const;

private:
    std::unique_ptr<SessionStorage> storage_;
    std::mt19937 rng_;
};

} // namespace session
} // namespace http
