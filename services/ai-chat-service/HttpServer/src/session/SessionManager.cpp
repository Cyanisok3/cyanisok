#include "../include/session/SessionManager.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace http
{
namespace session
{

SessionManager::SessionManager(std::unique_ptr<SessionStorage> storage)
    : storage_(std::move(storage))
    , rng_(std::random_device{}())
{}

std::shared_ptr<Session> SessionManager::getSession(const HttpRequest& req, HttpResponse* resp)
{
    std::string sessionId = getSessionIdFromCookie(req);
    std::shared_ptr<Session> session;

    if (!sessionId.empty())
    {
        session = storage_->load(sessionId);
    }

    if (!session || session->isExpired())
    {
        sessionId = generateSessionId();
        session = std::make_shared<Session>(sessionId, this);
        setSessionCookie(sessionId, resp);
    }
    else
    {
        session->setManager(this);
    }

    session->refresh();
    storage_->save(session);
    return session;
}

std::string SessionManager::generateSessionId()
{
    std::stringstream ss;
    std::uniform_int_distribution<> dist(0, 15);

    for (int i = 0; i < 32; ++i)
    {
        ss << std::hex << dist(rng_);
    }
    return ss.str();
}

void SessionManager::destroySession(const std::string& sessionId)
{
    storage_->remove(sessionId);
}

void SessionManager::clearSessionCookie(HttpResponse* resp)
{
    if (resp == nullptr)
    {
        return;
    }

    resp->addHeader("Set-Cookie", "sessionId=; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT" + cookieAttributes());
}

void SessionManager::cleanExpiredSessions()
{
}

std::string SessionManager::getSessionIdFromCookie(const HttpRequest& req)
{
    std::string sessionId;
    std::string cookie = req.getHeader("Cookie");

    if (!cookie.empty())
    {
        size_t pos = cookie.find("sessionId=");
        if (pos != std::string::npos)
        {
            pos += 10;
            size_t end = cookie.find(';', pos);
            if (end != std::string::npos)
            {
                sessionId = cookie.substr(pos, end - pos);
            }
            else
            {
                sessionId = cookie.substr(pos);
            }
        }
    }

    return sessionId;
}

void SessionManager::setSessionCookie(const std::string& sessionId, HttpResponse* resp)
{
    std::string cookie = "sessionId=" + sessionId + cookieAttributes();
    resp->addHeader("Set-Cookie", cookie);
}

std::string SessionManager::cookieAttributes() const
{
    std::string attributes = "; Path=/; HttpOnly; SameSite=Lax";
    const char* secure = std::getenv("SESSION_COOKIE_SECURE");
    if (secure != nullptr && std::string(secure) == "true")
    {
        attributes += "; Secure";
    }
    return attributes;
}

} // namespace session
} // namespace http
