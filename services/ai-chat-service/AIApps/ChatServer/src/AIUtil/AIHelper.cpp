#include "../../include/AIUtil/AIHelper.h"
#include "../../include/AIUtil/SseStreamParser.h"

#include <algorithm>
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
        LOG_WARN << "Ignoring invalid numeric env var " << key;
        return fallback;
    }
    return parsed;
}

struct StreamParseState
{
    explicit StreamParseState(const AIHelper::TokenCallback& onToken)
        : parser(onToken)
    {
    }

    SseStreamParser parser;
    std::string errorResponse;
    AIHelper::CancelCallback shouldCancel;
};

} // namespace

AIHelper::AIClientConfig AIHelper::AIClientConfig::fromEnvironment()
{
    AIClientConfig config;

    if (const char* model = std::getenv("AI_MODEL"); model && model[0] != '\0')
    {
        config.model = model;
    }
    if (const char* apiUrl = std::getenv("AI_API_URL");
        apiUrl && apiUrl[0] != '\0')
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
    config.maxTokens = readPositiveLongEnv(
        "AI_MAX_TOKENS",
        config.maxTokens);
    return config;
}

AIHelper::AIHelper(const std::string& apiKey)
    : AIHelper(apiKey, AIClientConfig::fromEnvironment())
{
}

AIHelper::AIHelper(const std::string& apiKey, const AIClientConfig& config)
    : apiKey_(apiKey)
    , config_(config)
{
}

std::string AIHelper::chatStream(
    const std::vector<ChatMessage>& messages,
    const TokenCallback& onToken,
    const CancelCallback& shouldCancel) const
{
    if (messages.empty())
    {
        throw std::invalid_argument("AI context cannot be empty");
    }

    const std::string answer = executeCurlStream(
        buildPayload(messages, true),
        onToken,
        shouldCancel);
    if (answer.empty())
    {
        throw std::runtime_error("AI returned an empty streaming response");
    }
    return answer;
}

json AIHelper::buildPayload(
    const std::vector<ChatMessage>& messages,
    bool stream) const
{
    json payload;
    payload["model"] = config_.model;
    payload["messages"] = json::array();
    for (const auto& message : messages)
    {
        payload["messages"].push_back({
            {"role", message.isUser ? "user" : "assistant"},
            {"content", message.content}
        });
    }
    payload["stream"] = stream;
    payload["max_tokens"] = config_.maxTokens;
    return payload;
}

std::string AIHelper::executeCurlStream(
    const json& payload,
    const TokenCallback& onToken,
    const CancelCallback& shouldCancel) const
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        throw std::runtime_error("Failed to initialize curl");
    }

    StreamParseState state(onToken);
    state.shouldCancel = shouldCancel;

    struct curl_slist* headers = nullptr;
    const std::string authHeader = "Authorization: Bearer " + apiKey_;
    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: text/event-stream");

    const std::string payloadString = payload.dump();
    curl_easy_setopt(curl, CURLOPT_URL, config_.apiUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadString.c_str());
    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDSIZE,
        static_cast<long>(payloadString.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config_.connectTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.requestTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, config_.streamIdleTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &state);

    const CURLcode result = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    state.parser.finish();

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (!state.parser.error().empty())
    {
        throw std::runtime_error(state.parser.error());
    }
    if (result == CURLE_ABORTED_BY_CALLBACK && shouldCancel && shouldCancel())
    {
        throw std::runtime_error("Client disconnected");
    }
    if (result != CURLE_OK)
    {
        throw std::runtime_error(
            "AI request failed: " + std::string(curl_easy_strerror(result)));
    }
    if (httpCode != 200)
    {
        throw std::runtime_error(
            "AI API HTTP error " + std::to_string(httpCode) +
            ": " + state.errorResponse);
    }
    if (!state.parser.done())
    {
        throw std::runtime_error(
            "AI stream ended before the completion marker");
    }
    return state.parser.answer();
}

size_t AIHelper::StreamWriteCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userp)
{
    const size_t totalSize = size * nmemb;
    auto* state = static_cast<StreamParseState*>(userp);
    const std::string chunk(static_cast<char*>(contents), totalSize);
    if (state->errorResponse.size() < 65536)
    {
        state->errorResponse.append(
            chunk,
            0,
            std::min(chunk.size(), 65536 - state->errorResponse.size()));
    }
    state->parser.append(chunk.data(), chunk.size());
    return state->parser.error().empty() ? totalSize : 0;
}

int AIHelper::ProgressCallback(
    void* clientp,
    curl_off_t,
    curl_off_t,
    curl_off_t,
    curl_off_t)
{
    auto* state = static_cast<StreamParseState*>(clientp);
    return state->shouldCancel && state->shouldCancel() ? 1 : 0;
}
