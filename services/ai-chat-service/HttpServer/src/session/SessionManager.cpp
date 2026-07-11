#include "../include/session/SessionManager.h"
#include <cctype>
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
    auto session = findSession(req);
    if (!session)
    {
        session = createSession(resp);
    }

    return session;
}

std::shared_ptr<Session> SessionManager::findSession(const HttpRequest& req)
{
    cleanExpiredSessionsIfDue();

    const std::string sessionId = getSessionIdFromCookie(req);
    if (sessionId.empty())
    {
        return nullptr;
    }

    auto session = storage_->load(sessionId);
    if (!session)
    {
        return nullptr;
    }

    session->refresh();
    storage_->save(session);
    return session;
}

void SessionManager::cleanExpiredSessionsIfDue()
{
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long expected = nextCleanupAtMs_.load();
    if (nowMs >= expected &&
        nextCleanupAtMs_.compare_exchange_strong(expected, nowMs + 300000))
    {
        cleanExpiredSessions();
    }
}

std::shared_ptr<Session> SessionManager::rotateSession(
    const HttpRequest& req,
    HttpResponse* resp)
{
    const std::string previousSessionId = getSessionIdFromCookie(req);
    if (!previousSessionId.empty())
    {
        const auto previousSession = storage_->load(previousSessionId);
        if (previousSession)
        {
            previousSession->clear();
        }
        storage_->remove(previousSessionId);
    }

    return createSession(resp);
}

std::shared_ptr<Session> SessionManager::createSession(HttpResponse* resp)
{
    const std::string sessionId = generateSessionId();
    auto session = std::make_shared<Session>(
        sessionId,
        this,
        sessionMaxAge());
    storage_->save(session);
    setSessionCookie(sessionId, resp);
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
    const std::string cookie = req.getHeader("Cookie");
    size_t start = 0;
    while (start < cookie.size())
    {
        const size_t end = cookie.find(';', start);
        const size_t itemEnd = end == std::string::npos ? cookie.size() : end;
        const size_t equals = cookie.find('=', start);
        if (equals != std::string::npos && equals < itemEnd)
        {
            size_t keyStart = start;
            while (keyStart < equals &&
                   std::isspace(static_cast<unsigned char>(cookie[keyStart])))
            {
                ++keyStart;
            }
            size_t keyEnd = equals;
            while (keyEnd > keyStart &&
                   std::isspace(static_cast<unsigned char>(cookie[keyEnd - 1])))
            {
                --keyEnd;
            }
            if (cookie.substr(keyStart, keyEnd - keyStart) == "sessionId")
            {
                return cookie.substr(equals + 1, itemEnd - equals - 1);
            }
        }

        if (end == std::string::npos)
        {
            break;
        }
        start = end + 1;
    }
    return "";
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
