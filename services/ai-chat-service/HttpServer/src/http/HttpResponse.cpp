#include "../../include/http/HttpResponse.h"

#include <algorithm>
#include <cctype>

namespace http
{

namespace
{

bool equalsIgnoreCase(const std::string &left, const std::string &right)
{
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(), [](unsigned char a, unsigned char b) {
               return std::tolower(a) == std::tolower(b);
           });
}

} // namespace

void HttpResponse::appendToBuffer(muduo::net::Buffer* outputBuf) const
{
    char buf[32];
    snprintf(buf, sizeof buf, "%s %d ", httpVersion_.c_str(), statusCode_);

    outputBuf->append(buf);
    outputBuf->append(statusMessage_);
    outputBuf->append("\r\n");

    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    outputBuf->append(buf);

    if (closeConnection_)
    {
        outputBuf->append("Connection: close\r\n");
    }
    else
    {
        outputBuf->append("Connection: Keep-Alive\r\n");
    }

    for (const auto& header : headers_)
    {
        if (equalsIgnoreCase(header.first, "Content-Length"))
        {
            continue;
        }
        outputBuf->append(header.first);
        outputBuf->append(": ");
        outputBuf->append(header.second);
        outputBuf->append("\r\n");
    }
    outputBuf->append("\r\n");

    outputBuf->append(body_);
}

void HttpResponse::setStatusLine(const std::string& version,
                                 HttpStatusCode statusCode,
                                 const std::string& statusMessage)
{
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

} // namespace http
