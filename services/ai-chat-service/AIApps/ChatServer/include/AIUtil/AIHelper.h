#pragma once

#include <string>
#include <vector>
#include <curl/curl.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>

#include "../../../../HttpServer/include/utils/JsonUtil.h"
#include "../../../../HttpServer/include/utils/MysqlUtil.h"

// Wrapper for DashScope AI API calls via curl
class AIHelper {
public:
    enum class ChatRole {
        User,
        Assistant
    };

    struct ChatMessage {
        ChatRole role;
        std::string content;
        long long timestampMs;
    };

    struct AIClientConfig {
        std::string model = "qwen-plus";
        std::string apiUrl = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
        long requestTimeoutSeconds = 60;
        long connectTimeoutSeconds = 10;
        long streamIdleTimeoutSeconds = 90;

        static AIClientConfig fromEnvironment();
    };

    using TokenCallback = std::function<void(const std::string&)>;

    AIHelper(const std::string& apiKey);
    AIHelper(const std::string& apiKey, const AIClientConfig& config);

    void setModel(const std::string& modelName);

    void addMessage(int userId, const std::string& userName, bool is_user, const std::string& userInput);
    void restoreMessage(bool isUser, const std::string& userInput, long long ms);

    std::string chat(int userId, std::string userName);
    std::string chatStream(int userId, const std::string& userName, const TokenCallback& onToken);

    json request(const json& payload);

    std::vector<ChatMessage> getMessages() const;

private:
    static long long currentTimestampMs();
    static const char* roleToString(ChatRole role);
    static ChatRole roleFromIsUser(bool isUser);

    void appendMessage(ChatRole role, const std::string& content, long long timestampMs);
    json buildPayload(bool stream = false) const;
    void pushMessageToMysql(int userId, const std::string& userName, bool is_user, const std::string& userInput, long long ms);
    json executeCurl(const json& payload);
    std::string executeCurlStream(const json& payload, const TokenCallback& onToken);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t StreamWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

private:
    std::string apiKey_;
    AIClientConfig config_;
    std::vector<ChatMessage> messages_;
    mutable std::mutex messagesMutex_;
};
