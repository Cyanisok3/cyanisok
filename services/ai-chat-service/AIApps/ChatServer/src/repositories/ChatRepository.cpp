#include "../../include/repositories/ChatRepository.h"

#include <algorithm>

void ChatRepository::append(
    int userId,
    const std::string& username,
    bool isUser,
    const std::string& content,
    long long timestampMs) const
{
    mysql_.executeUpdate(
        "INSERT INTO chat_message "
        "(user_id, username, is_user, content, ts) VALUES (?, ?, ?, ?, ?)",
        userId,
        username,
        isUser,
        content,
        timestampMs);
}

std::vector<ChatMessage> ChatRepository::recent(int userId, int limit) const
{
    if (limit <= 0)
    {
        return {};
    }

    return mysql_.query(
        "SELECT is_user, content, ts FROM ("
        "SELECT message_id, is_user, content, ts "
        "FROM chat_message WHERE user_id = ? "
        "ORDER BY ts DESC, message_id DESC LIMIT ?"
        ") recent_messages ORDER BY ts ASC, message_id ASC",
        [](sql::ResultSet& result) {
            std::vector<ChatMessage> messages;
            while (result.next())
            {
                messages.push_back(ChatMessage{
                    result.getBoolean("is_user"),
                    result.getString("content"),
                    result.getInt64("ts")
                });
            }
            return messages;
        },
        userId,
        limit);
}

int ChatRepository::deleteOlderThan(
    long long cutoffTimestampMs,
    int batchSize) const
{
    return mysql_.executeUpdate(
        "DELETE FROM chat_message WHERE ts < ? LIMIT ?",
        cutoffTimestampMs,
        batchSize);
}

bool ChatRepository::ping() const
{
    try
    {
        return mysql_.ping();
    }
    catch (...)
    {
        return false;
    }
}
