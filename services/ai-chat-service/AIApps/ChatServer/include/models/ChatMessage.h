#pragma once

#include <string>

struct ChatMessage
{
    bool isUser;
    std::string content;
    long long timestampMs;
};
