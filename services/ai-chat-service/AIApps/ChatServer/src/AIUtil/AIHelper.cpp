#include "../../include/AIUtil/AIHelper.h"
#include "../../include/AIUtil/MQManager.h"

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include <muduo/base/Logging.h>

namespace
{

long readPositiveLongEnv(const char* key, long fallback)
{
    const char* rawValue = std::getenv(key);
    if (!rawValue || rawValue[0] == '\0')
    {
        return fallback;
    }

    char* end = nullptr;
    long parsed = std::strtol(rawValue, &end, 10);
    if (end == rawValue || parsed <= 0)
    {
        LOG_WARN << "Ignoring invalid numeric env var " << key << "=" << rawValue;
        return fallback;
    }

    return parsed;
}

std::string trimSseValue(std::string value)
{
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
    {
        value.erase(value.begin());
    }
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n'))
    {
        value.pop_back();
    }
    return value;
}

struct StreamParseState
{
    std::string pending;
    std::string answer;
    std::string rawResponse;
    std::string error;
    bool done = false;
    AIHelper::TokenCallback onToken;
};

void handleStreamDataLine(StreamParseState& state, const std::string& data)
{
    if (data.empty())
    {
        return;
    }

    if (data == "[DONE]")
    {
        state.done = true;
        return;
    }

    try
    {
        json payload = json::parse(data);
        if (payload.contains("error"))
        {
            if (payload["error"].contains("message"))
            {
                state.error = payload["error"]["message"].get<std::string>();
            }
            else
            {
                state.error = payload["error"].dump();
            }
            return;
        }

        if (!payload.contains("choices") || payload["choices"].empty())
        {
            return;
        }

        const auto& choice = payload["choices"][0];
        if (choice.contains("delta") &&
            choice["delta"].contains("content") &&
            choice["delta"]["content"].is_string())
        {
            std::string token = choice["delta"]["content"].get<std::string>();
            if (!token.empty())
            {
                state.answer += token;
                if (state.onToken)
                {
                    state.onToken(token);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        state.error = std::string("Failed to parse streaming response: ") + e.what();
    }
}

void processStreamBuffer(StreamParseState& state)
{
    size_t newline = std::string::npos;
    while ((newline = state.pending.find('\n')) != std::string::npos)
    {
        std::string line = state.pending.substr(0, newline);
        state.pending.erase(0, newline + 1);
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.rfind("data:", 0) == 0)
        {
            handleStreamDataLine(state, trimSseValue(line.substr(5)));
            if (!state.error.empty())
            {
                return;
            }
        }
    }
}

} // namespace

AIHelper::AIClientConfig AIHelper::AIClientConfig::fromEnvironment()
{
    AIClientConfig config;

    const char* model = std::getenv("AI_MODEL");
    if (model && model[0] != '\0')
    {
        config.model = model;
    }

    const char* apiUrl = std::getenv("AI_API_URL");
    if (apiUrl && apiUrl[0] != '\0')
    {
        config.apiUrl = apiUrl;
    }

    config.requestTimeoutSeconds = readPositiveLongEnv(
        "AI_REQUEST_TIMEOUT_SECONDS",
        config.requestTimeoutSeconds);
    config.connectTimeoutSeconds = readPositiveLongEnv(
        "AI_CONNECT_TIMEOUT_SECONDS",
        config.connectTimeoutSeconds);
    config.streamIdleTimeoutSeconds = readPositiveLongEnv(
        "AI_STREAM_IDLE_TIMEOUT_SECONDS",
        config.streamIdleTimeoutSeconds);

    return config;
}

AIHelper::AIHelper(const std::string& apiKey)
    : AIHelper(apiKey, AIClientConfig::fromEnvironment()) {
}

AIHelper::AIHelper(const std::string& apiKey, const AIClientConfig& config)
    : apiKey_(apiKey)
    , config_(config) {
}

void AIHelper::setModel(const std::string& modelName) {
    config_.model = modelName;
}

void AIHelper::addMessage(int userId, const std::string& userName, bool is_user, const std::string& userInput) {
    auto ms = currentTimestampMs();
    std::lock_guard<std::mutex> lock(messagesMutex_);
    appendMessage(roleFromIsUser(is_user), userInput, ms);
    pushMessageToMysql(userId, userName, is_user, userInput, ms);
}

void AIHelper::restoreMessage(bool isUser, const std::string& userInput, long long ms) {
    std::lock_guard<std::mutex> lock(messagesMutex_);
    appendMessage(roleFromIsUser(isUser), userInput, ms);
}

std::string AIHelper::chat(int userId, std::string userName) {
    json payload = buildPayload(false);
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

std::string AIHelper::chatStream(int userId, const std::string& userName, const TokenCallback& onToken) {
    json payload = buildPayload(true);
    std::string answer = executeCurlStream(payload, onToken);
    if (answer.empty()) {
        throw std::runtime_error("AI returned an empty streaming response");
    }
    addMessage(userId, userName, false, answer);
    return answer;
}

json AIHelper::request(const json& payload) {
    return executeCurl(payload);
}

std::vector<AIHelper::ChatMessage> AIHelper::getMessages() const {
    std::lock_guard<std::mutex> lock(messagesMutex_);
    return messages_;
}

long long AIHelper::currentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

const char* AIHelper::roleToString(ChatRole role) {
    return role == ChatRole::User ? "user" : "assistant";
}

AIHelper::ChatRole AIHelper::roleFromIsUser(bool isUser) {
    return isUser ? ChatRole::User : ChatRole::Assistant;
}

void AIHelper::appendMessage(ChatRole role, const std::string& content, long long timestampMs) {
    messages_.push_back({ role, content, timestampMs });
}

json AIHelper::buildPayload(bool stream) const {
    json payload;
    payload["model"] = config_.model;
    payload["messages"] = json::array();

    std::lock_guard<std::mutex> lock(messagesMutex_);
    for (const auto& message : messages_) {
        json msg;
        msg["role"] = roleToString(message.role);
        msg["content"] = message.content;
        payload["messages"].push_back(msg);
    }

    if (stream) {
        payload["stream"] = true;
    }

    return payload;
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
    curl_easy_setopt(curl, CURLOPT_URL, config_.apiUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payloadStr.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.requestTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config_.connectTimeoutSeconds);

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

std::string AIHelper::executeCurlStream(const json& payload, const TokenCallback& onToken) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    StreamParseState state;
    state.onToken = onToken;

    struct curl_slist* headers = nullptr;
    std::string authHeader = "Authorization: Bearer " + apiKey_;

    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: text/event-stream");

    std::string payloadStr = payload.dump();
    curl_easy_setopt(curl, CURLOPT_URL, config_.apiUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payloadStr.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config_.connectTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, config_.streamIdleTimeoutSeconds);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    if (!state.pending.empty() && state.error.empty())
    {
        std::string line = state.pending;
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.rfind("data:", 0) == 0)
        {
            handleStreamDataLine(state, trimSseValue(line.substr(5)));
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (!state.error.empty()) {
        throw std::runtime_error(state.error);
    }

    if (res != CURLE_OK) {
        throw std::runtime_error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
    }

    if (httpCode != 200) {
        throw std::runtime_error("AI API HTTP error, code: " + std::to_string(httpCode) + ", response: " + state.rawResponse);
    }

    return state.answer;
}

size_t AIHelper::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

size_t AIHelper::StreamWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* state = static_cast<StreamParseState*>(userp);
    std::string chunk(static_cast<char*>(contents), totalSize);
    state->rawResponse += chunk;
    state->pending += chunk;
    processStreamBuffer(*state);

    if (!state->error.empty())
    {
        return 0;
    }

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
