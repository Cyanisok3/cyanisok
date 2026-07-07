#pragma once

#include <string>
#include <vector>

#include "../../../../HttpServer/include/utils/MysqlUtil.h"
#include "../models/ChatMessage.h"

class ChatRepository
{
public:
    void append(
        int userId,
        const std::string& username,
        bool isUser,
        const std::string& content,
        long long timestampMs) const;

    std::vector<ChatMessage> recent(int userId, int limit) const;
    int deleteOlderThan(long long cutoffTimestampMs, int batchSize) const;
    bool ping() const;

private:
    mutable http::MysqlUtil mysql_;
};
