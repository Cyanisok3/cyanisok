#pragma once

#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http
{
namespace middleware
{

class Middleware
{
public:
    virtual ~Middleware() = default;

    virtual void before(HttpRequest& request) = 0;

    virtual void after(HttpResponse& response) = 0;

    void setNext(std::shared_ptr<Middleware> next)
    {
        nextMiddleware_ = next;
    }

protected:
    std::shared_ptr<Middleware> nextMiddleware_;
};

} // namespace middleware
} // namespace http
