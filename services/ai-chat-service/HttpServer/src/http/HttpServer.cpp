#include "../../include/http/HttpServer.h"

#include <any>
#include <functional>
#include <memory>

namespace http
{

void defaultHttpCallback(const HttpRequest &req, HttpResponse *resp)
{
    resp->setVersion(req.getVersion() == "Unknown" ? "HTTP/1.1" : req.getVersion());
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(int port,
                       const std::string &name,
                       bool useSSL,
                       muduo::net::TcpServer::Option option)
    : listenAddr_(port)
    , server_(&mainLoop_, listenAddr_, name, option)
    , useSSL_(useSSL)
    , httpCallback_(std::bind(&HttpServer::handleRequest, this, std::placeholders::_1, std::placeholders::_2))
{
    initialize();
}

void HttpServer::start()
{
    LOG_WARN << "HttpServer[" << server_.name() << "] starts listening on" << server_.ipPort();
    server_.start();
    mainLoop_.loop();
}

void HttpServer::PostStream(const std::string& path, const StreamCallback& cb)
{
    streamCallbacks_[routeKey(HttpRequest::kPost, path)] = cb;
}

void HttpServer::initialize()
{
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));
}

void HttpServer::setSslConfig(const ssl::SslConfig& config)
{
    if (useSSL_)
    {
        sslCtx_ = std::make_unique<ssl::SslContext>(config);
        if (!sslCtx_->initialize())
        {
            LOG_ERROR << "Failed to initialize SSL context";
            abort();
        }
    }
}

void HttpServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        if (useSSL_)
        {
            auto sslConn = std::make_unique<ssl::SslConnection>(conn, sslCtx_.get());
            sslConn->setMessageCallback(
                std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            sslConns_[conn] = std::move(sslConn);
            sslConns_[conn]->startHandshake();
        }
        conn->setContext(HttpContext());
    }
    else
    {
        if (useSSL_)
        {
            sslConns_.erase(conn);
        }
    }
}

void HttpServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,
                           muduo::Timestamp receiveTime)
{
    try
    {
        if (useSSL_)
        {
            LOG_INFO << "onMessage useSSL_ is true";
            auto it = sslConns_.find(conn);
            if (it != sslConns_.end())
            {
                LOG_INFO << "onMessage sslConns_ is not empty";
                it->second->onRead(conn, buf, receiveTime);

                if (!it->second->isHandshakeCompleted())
                {
                    LOG_INFO << "onMessage sslConns_ is not empty";
                    return;
                }

                muduo::net::Buffer* decryptedBuf = it->second->getDecryptedBuffer();
                if (decryptedBuf->readableBytes() == 0)
                    return;

                buf = decryptedBuf;
                LOG_INFO << "onMessage decryptedBuf is not empty";
            }
        }

        HttpContext *context = boost::any_cast<HttpContext>(conn->getMutableContext());
        if (!context->parseRequest(buf, receiveTime))
        {
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }

        if (context->gotAll())
        {
            onRequest(conn, context->request());
            context->reset();
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Exception in onMessage: " << e.what();
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
}

void HttpServer::onRequest(const muduo::net::TcpConnectionPtr &conn, const HttpRequest &req)
{
    if (handleStreamRequest(conn, req))
    {
        return;
    }

    const std::string &connection = req.getHeader("Connection");
    bool close = ((connection == "close") ||
                  (req.getVersion() == "HTTP/1.0" && connection != "Keep-Alive"));
    HttpResponse response(close);

    httpCallback_(req, &response);

    muduo::net::Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if (response.closeConnection())
    {
        conn->shutdown();
    }
}

bool HttpServer::handleStreamRequest(const muduo::net::TcpConnectionPtr& conn, const HttpRequest& req)
{
    auto it = streamCallbacks_.find(routeKey(req.method(), req.path()));
    if (it == streamCallbacks_.end())
    {
        return false;
    }

    try
    {
        it->second(conn, req);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "EXCEPTION in stream request: " << e.what();
        conn->send("HTTP/1.1 500 Internal Server Error\r\n"
                   "Connection: close\r\n"
                   "Content-Type: text/plain\r\n"
                   "Content-Length: 21\r\n"
                   "\r\n"
                   "Internal Server Error");
        conn->shutdown();
    }

    return true;
}

std::string HttpServer::routeKey(HttpRequest::Method method, const std::string& path)
{
    return std::to_string(static_cast<int>(method)) + " " + path;
}

void HttpServer::handleRequest(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        HttpRequest mutableReq = req;
        middlewareChain_.processBefore(mutableReq);

        if (!router_.route(mutableReq, resp))
        {
            LOG_INFO << "Route not found: " << req.method() << " " << req.path();
            resp->setVersion(req.getVersion() == "Unknown" ? "HTTP/1.1" : req.getVersion());
            resp->setStatusCode(HttpResponse::k404NotFound);
            resp->setStatusMessage("Not Found");
            resp->setCloseConnection(true);
        }

        middlewareChain_.processAfter(*resp);
    }
    catch (const HttpResponse& res)
    {
        *resp = res;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "EXCEPTION in handleRequest: " << e.what();
        resp->setVersion(req.getVersion() == "Unknown" ? "HTTP/1.1" : req.getVersion());
        resp->setStatusCode(HttpResponse::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setCloseConnection(true);
        resp->setBody(e.what());
    }
}

} // namespace http
