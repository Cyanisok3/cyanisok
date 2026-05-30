#pragma once

#include <string>
#include <vector>
#include <utility>
#include <curl/curl.h>
#include <iostream>
#include <sstream>

#include "../../../../HttpServer/include/utils/JsonUtil.h"
#include "../../../../HttpServer/include/utils/MysqlUtil.h"

// Wrapper for DashScope AI API calls via curl
class AIHelper {
public:
    AIHelper(const std::string& apiKey);

    void setModel(const std::string& modelName);

    void addMessage(int userId, const std::string& userName, bool is_user, const std::string& userInput);
    void restoreMessage(const std::string& userInput, long long ms);

    std::string chat(int userId, std::string userName);

    json request(const json& payload);

    std::vector<std::pair<std::string, long long>> GetMessages();

private:
    void pushMessageToMysql(int userId, const std::string& userName, bool is_user, const std::string& userInput, long long ms);
    json executeCurl(const json& payload);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

private:
    std::string apiKey_;
    std::string model_ = "qwen-plus";
    std::string apiUrl_ = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
    std::vector<std::pair<std::string, long long>> messages;
};
