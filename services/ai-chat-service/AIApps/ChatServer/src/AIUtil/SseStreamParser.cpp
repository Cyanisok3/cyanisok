#include "../../include/AIUtil/SseStreamParser.h"

#include <utility>

#include "../../../../HttpServer/include/utils/JsonUtil.h"

namespace
{

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

} // namespace

SseStreamParser::SseStreamParser(TokenCallback onToken)
    : onToken_(std::move(onToken))
{
}

void SseStreamParser::append(const char* data, size_t size)
{
    if (size == 0 || !error_.empty())
    {
        return;
    }
    pending_.append(data, size);
    processPending(false);
}

void SseStreamParser::finish()
{
    processPending(true);
}

const std::string& SseStreamParser::answer() const
{
    return answer_;
}

const std::string& SseStreamParser::error() const
{
    return error_;
}

bool SseStreamParser::done() const
{
    return done_;
}

void SseStreamParser::processPending(bool flush)
{
    size_t newline = std::string::npos;
    while ((newline = pending_.find('\n')) != std::string::npos)
    {
        std::string line = pending_.substr(0, newline);
        pending_.erase(0, newline + 1);
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.rfind("data:", 0) == 0)
        {
            handleDataLine(trimSseValue(line.substr(5)));
            if (!error_.empty())
            {
                return;
            }
        }
    }

    if (flush && !pending_.empty())
    {
        std::string line = std::move(pending_);
        pending_.clear();
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.rfind("data:", 0) == 0)
        {
            handleDataLine(trimSseValue(line.substr(5)));
        }
    }
}

void SseStreamParser::handleDataLine(const std::string& data)
{
    if (data.empty())
    {
        return;
    }
    if (data == "[DONE]")
    {
        done_ = true;
        return;
    }

    try
    {
        const json payload = json::parse(data);
        if (payload.contains("error"))
        {
            error_ = payload["error"].contains("message")
                ? payload["error"]["message"].get<std::string>()
                : payload["error"].dump();
            return;
        }
        if (!payload.contains("choices") || payload["choices"].empty())
        {
            return;
        }

        const auto& choice = payload["choices"][0];
        if (!choice.contains("delta") ||
            !choice["delta"].contains("content") ||
            !choice["delta"]["content"].is_string())
        {
            return;
        }

        const std::string token =
            choice["delta"]["content"].get<std::string>();
        if (token.empty())
        {
            return;
        }

        answer_ += token;
        if (onToken_)
        {
            onToken_(token);
        }
    }
    catch (const std::exception& e)
    {
        error_ = std::string("Failed to parse streaming response: ") + e.what();
    }
}
