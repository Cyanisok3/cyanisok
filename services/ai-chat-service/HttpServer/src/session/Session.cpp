#include "../include/session/Session.h"
#include "../include/session/SessionManager.h"

namespace http
{
namespace session
{

Session::Session(const std::string& sessionId, SessionManager* sessionManager, int maxAge)
    : sessionId_(sessionId)
    , maxAge_(maxAge)
    , sessionManager_(sessionManager)
{
    refresh();
}

bool Session::isExpired() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return std::chrono::system_clock::now() > expiryTime_;
}

void Session::refresh()
{
    std::lock_guard<std::mutex> lock(mutex_);
    expiryTime_ = std::chrono::system_clock::now() + std::chrono::seconds(maxAge_);
}

void Session::setValue(const std::string& key, const std::string& value)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = value;
    }
    if (sessionManager_)
    {
        sessionManager_->updateSession(shared_from_this());
    }
}

std::string Session::getValue(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    return it != data_.end() ? it->second : std::string();
}

void Session::remove(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    data_.erase(key);
}

void Session::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
}

} // namespace session
} // namespace http
