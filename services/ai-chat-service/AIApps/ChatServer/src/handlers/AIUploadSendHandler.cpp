#include "../include/handlers/AIUploadSendHandler.h"
#include "../include/AIUtil/ImageValidation.h"
#include "../include/AIUtil/base64.h"

namespace
{

std::string statusText(http::HttpResponse::HttpStatusCode statusCode)
{
    switch (statusCode)
    {
    case http::HttpResponse::k400BadRequest:
        return "Bad Request";
    case http::HttpResponse::k401Unauthorized:
        return "Unauthorized";
    case http::HttpResponse::k503ServiceUnavailable:
        return "Service Unavailable";
    case http::HttpResponse::k500InternalServerError:
        return "Internal Server Error";
    default:
        return "OK";
    }
}

void sendJsonOnLoop(
    const std::weak_ptr<muduo::net::TcpConnection>& weakConn,
    const std::string& version,
    http::HttpResponse::HttpStatusCode statusCode,
    const json& body)
{
    const std::string responseBody = body.dump(4);
    const std::string httpVersion =
        version.empty() || version == "Unknown" ? "HTTP/1.1" : version;
    const std::string response =
        httpVersion + " " +
        std::to_string(static_cast<int>(statusCode)) + " " +
        statusText(statusCode) + "\r\n" +
        "Connection: close\r\n" +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + std::to_string(responseBody.size()) + "\r\n" +
        "Cache-Control: no-store\r\n\r\n" +
        responseBody;

    const auto conn = weakConn.lock();
    if (!conn)
    {
        return;
    }
    conn->getLoop()->runInLoop([weakConn, response] {
        const auto liveConn = weakConn.lock();
        if (liveConn && liveConn->connected())
        {
            liveConn->send(response);
            liveConn->shutdown();
        }
    });
}

} // namespace

void AIUploadSendHandler::handleStream(
    const muduo::net::TcpConnectionPtr& conn,
    const http::HttpRequest& req)
{
    try
    {
        const auto session =
            server_->getSessionManager()->findSession(req);
        if (!session || session->getValue("isLoggedIn") != "true")
        {
            sendJsonOnLoop(
                conn,
                req.getVersion(),
                http::HttpResponse::k401Unauthorized,
                {{"status", "error"}, {"message", "Unauthorized"}});
            return;
        }

        if (!server_->imageRecognizer_ || !server_->imageExecutor_)
        {
            sendJsonOnLoop(
                conn,
                req.getVersion(),
                http::HttpResponse::k503ServiceUnavailable,
                {
                    {"status", "error"},
                    {"message", "Image recognition is unavailable"}
                });
            return;
        }

        const json parsed = json::parse(req.getBody());
        const std::string filename = parsed.value("filename", "");
        const std::string imageBase64 = parsed.value("image", "");
        if (imageBase64.empty())
        {
            throw std::runtime_error("No image data provided");
        }
        if (imageBase64.size() > image_validation::kMaxBase64Characters)
        {
            throw std::runtime_error("Image exceeds the 6MB encoded size limit");
        }

        const std::string version = req.getVersion();
        const std::weak_ptr<muduo::net::TcpConnection> weakConn = conn;
        const bool submitted = server_->imageExecutor_->trySubmit(
            [server = server_,
             weakConn,
             version,
             filename,
             imageBase64] {
                if (weakConn.expired())
                {
                    return;
                }

                try
                {
                    const std::string decodedData = base64_decode(imageBase64);
                    const std::vector<uchar> imageData(
                        decodedData.begin(),
                        decodedData.end());
                    const auto prediction =
                        server->imageRecognizer_->PredictFromBuffer(imageData);

                    sendJsonOnLoop(
                        weakConn,
                        version,
                        http::HttpResponse::k200Ok,
                        {
                            {"success", "ok"},
                            {"filename", filename},
                            {"class_name", prediction.className},
                            {"confidence", prediction.confidence}
                        });
                }
                catch (const std::exception& e)
                {
                    sendJsonOnLoop(
                        weakConn,
                        version,
                        http::HttpResponse::k400BadRequest,
                        {{"status", "error"}, {"message", e.what()}});
                }
            });

        if (!submitted)
        {
            sendJsonOnLoop(
                conn,
                version,
                http::HttpResponse::k503ServiceUnavailable,
                {
                    {"status", "error"},
                    {"message", "Image service is busy; try again shortly"}
                });
        }
    }
    catch (const std::exception& e)
    {
        sendJsonOnLoop(
            conn,
            req.getVersion(),
            http::HttpResponse::k400BadRequest,
            {{"status", "error"}, {"message", e.what()}});
    }
}
