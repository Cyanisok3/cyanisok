#pragma once
#include <stdexcept>
#include <string>

namespace http {
namespace db {

class DbException : public std::runtime_error 
{
public:
    explicit DbException(const std::string& message, int errorCode = 0)
        : std::runtime_error(message)
        , errorCode_(errorCode) {}
    
    explicit DbException(const char* message, int errorCode = 0)
        : std::runtime_error(message)
        , errorCode_(errorCode) {}

    int errorCode() const noexcept
    {
        return errorCode_;
    }

private:
    int errorCode_;
};

} // namespace db
} // namespace http
