#include "../include/session/SessionManager.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <openssl/rand.h>
#include <sstream>
#include <stdexcept>

namespace http
{
namespace session
{

SessionManager::SessionManager(std::unique_ptr<SessionStorage> storage)
    : storage_(std::move(storage))
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
        session = std::make_shared<Session>(sessionId, this, sessionMaxAge());
        setSessionCookie(sessionId, resp);
    }
    session->refresh();
    storage_->save(session);

    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long expected = nextCleanupAtMs_.load();
    if (nowMs >= expected &&
        nextCleanupAtMs_.compare_exchange_strong(expected, nowMs + 300000))
    {
        cleanExpiredSessions();
    }

    return session;
}

std::string SessionManager::generateSessionId()
{
    unsigned char bytes[32];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1)
    {
        throw std::runtime_error("Unable to generate secure session id");
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char byte : bytes)
    {
        ss << std::setw(2) << static_cast<int>(byte);
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
    storage_->cleanExpired();
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
    if (resp == nullptr)
    {
        return;
    }
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

int SessionManager::sessionMaxAge() const
{
    const char* raw = std::getenv("SESSION_TTL_SECONDS");
    if (!raw || raw[0] == '\0')
    {
        return 3600;
    }

    try
    {
        int value = std::stoi(raw);
        return value > 0 ? value : 3600;
    }
    catch (...)
    {
        return 3600;
    }
}

} // namespace session
} // namespace http
