#include "../include/handlers/AIUploadSendHandler.h"
#include "../include/AIUtil/ImageValidation.h"
#include "../include/AIUtil/base64.h"

void AIUploadSendHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try
    {
        auto session = server_->getSessionManager()->getSession(req, resp);
        if (session->getValue("isLoggedIn") != "true")
        {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                "Unauthorized", true, "application/json", errorBody.size(),
                errorBody, resp);
            return;
        }

        if (!server_->imageRecognizer_)
        {
            json unavailable;
            unavailable["status"] = "error";
            unavailable["message"] = "Image recognition is unavailable";
            const std::string body = unavailable.dump(4);
            resp->setStatusLine(
                req.getVersion(),
                http::HttpResponse::k503ServiceUnavailable,
                "Service Unavailable");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }

        auto body = req.getBody();
        std::string filename;
        std::string imageBase64;
        if (!body.empty()) {
            auto j = json::parse(body);
            if (j.contains("filename")) filename = j["filename"];
            if (j.contains("image")) imageBase64 = j["image"];
        }
        if (imageBase64.empty())
        {
            throw std::runtime_error("No image data provided");
        }
        if (imageBase64.size() > image_validation::kMaxBase64Characters)
        {
            throw std::runtime_error("Image exceeds the 6MB encoded size limit");
        }

        std::string decodedData = base64_decode(imageBase64);
        std::vector<uchar> imgData(decodedData.begin(), decodedData.end());
        ImageRecognizer::PredictionResult prediction;
        {
            std::lock_guard<std::mutex> lock(server_->imageRecognizerMutex_);
            prediction = server_->imageRecognizer_->PredictFromBuffer(imgData);
        }

        json successResp;
        successResp["success"] = "ok";
        successResp["filename"] = filename;
        successResp["class_name"] = prediction.className;
        successResp["confidence"] = prediction.confidence;

        std::string successBody = successResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
        return;
    }
    catch (const std::exception& e)
    {
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}
