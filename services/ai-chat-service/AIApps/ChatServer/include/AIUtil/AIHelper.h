#pragma once

#include <curl/curl.h>

#include <functional>
#include <string>
#include <vector>

#include "../../../../HttpServer/include/utils/JsonUtil.h"
#include "../models/ChatMessage.h"

class AIHelper
{
public:
    struct AIClientConfig
    {
        std::string model = "qwen-plus";
        std::string apiUrl =
            "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
        long requestTimeoutSeconds = 60;
        long connectTimeoutSeconds = 10;
        long streamIdleTimeoutSeconds = 90;

        static AIClientConfig fromEnvironment();
    };

    using TokenCallback = std::function<void(const std::string&)>;
    using CancelCallback = std::function<bool()>;

    explicit AIHelper(const std::string& apiKey);
    AIHelper(const std::string& apiKey, const AIClientConfig& config);

    std::string chatStream(
        const std::vector<ChatMessage>& messages,
        const TokenCallback& onToken,
        const CancelCallback& shouldCancel = {}) const;

private:
    json buildPayload(
        const std::vector<ChatMessage>& messages,
        bool stream) const;
    std::string executeCurlStream(
        const json& payload,
        const TokenCallback& onToken,
        const CancelCallback& shouldCancel) const;

    static size_t StreamWriteCallback(
        void* contents,
        size_t size,
        size_t nmemb,
        void* userp);
    static int ProgressCallback(
        void* clientp,
        curl_off_t,
        curl_off_t,
        curl_off_t,
        curl_off_t);

    std::string apiKey_;
    AIClientConfig config_;
};
