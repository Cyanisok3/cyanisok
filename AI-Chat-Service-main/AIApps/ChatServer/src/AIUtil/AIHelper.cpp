#include "../../include/AIUtil/AIHelper.h"
#include "../../include/AIUtil/MQManager.h"
#include <stdexcept>
#include <chrono>
#include <muduo/base/Logging.h>

AIHelper::AIHelper(const std::string& apiKey)
    : apiKey_(apiKey) {
}

void AIHelper::setModel(const std::string& modelName) {
    model_ = modelName;
}

void AIHelper::addMessage(int userId, const std::string& userName, bool is_user, const std::string& userInput) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    messages.push_back({ userInput, ms });
    pushMessageToMysql(userId, userName, is_user, userInput, ms);
}

void AIHelper::restoreMessage(const std::string& userInput, long long ms) {
    messages.push_back({ userInput, ms });
}

std::string AIHelper::chat(int userId, std::string userName) {
    json payload;
    payload["model"] = model_;
    json msgArray = json::array();

    for (size_t i = 0; i < messages.size(); ++i) {
        json msg;
        if (i % 2 == 0) {
            msg["role"] = "user";
            msg["content"] = messages[i].first;
        }
        else {
            msg["role"] = "assistant";
            msg["content"] = messages[i].first;
        }
        msgArray.push_back(msg);
    }

    payload["messages"] = msgArray;

    json response = executeCurl(payload);

    if (response.contains("error")) {
        std::string errorMsg = "[Error] ";
        if (response["error"].contains("message")) {
            errorMsg += response["error"]["message"].get<std::string>();
        } else {
            errorMsg += response["error"].dump();
        }
        LOG_ERROR << "AI API Error: " << errorMsg;
        return errorMsg;
    }

    if (!response.contains("choices") || response["choices"].empty()) {
        LOG_ERROR << "AI API response missing or empty choices: " << response.dump();
        return "[Error] AI API response format error";
    }

    if (!response["choices"][0].contains("message") || !response["choices"][0]["message"].contains("content")) {
        LOG_ERROR << "AI API response missing message content: " << response.dump();
        return "[Error] AI response content missing";
    }

    std::string answer = response["choices"][0]["message"]["content"];
    addMessage(userId, userName, false, answer);
    return answer;
}

json AIHelper::request(const json& payload) {
    return executeCurl(payload);
}

std::vector<std::pair<std::string, long long>> AIHelper::GetMessages() {
    return this->messages;
}

json AIHelper::executeCurl(const json& payload) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    std::string readBuffer;
    struct curl_slist* headers = nullptr;
    std::string authHeader = "Authorization: Bearer " + apiKey_;

    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    std::string payloadStr = payload.dump();
    curl_easy_setopt(curl, CURLOPT_URL, apiUrl_.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
    }

    if (httpCode != 200) {
        LOG_ERROR << "AI API HTTP error, code: " << httpCode << ", response: " << readBuffer;
    }

    try {
        return json::parse(readBuffer);
    }
    catch (...) {
        throw std::runtime_error("Failed to parse JSON response: " + readBuffer);
    }
}

size_t AIHelper::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void AIHelper::pushMessageToMysql(int userId, const std::string& userName, bool is_user, const std::string& userInput, long long ms) {
    json message;
    message["type"] = "insert_chat_message";
    message["user_id"] = userId;
    message["username"] = userName;
    message["is_user"] = is_user;
    message["content"] = userInput;
    message["ts"] = ms;

    MQManager::instance().publish("sql_queue", message.dump());
}
