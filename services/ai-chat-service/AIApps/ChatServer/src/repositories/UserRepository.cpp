#include "../../include/repositories/UserRepository.h"

#include "../../../../HttpServer/include/utils/db/DbException.h"

std::optional<UserRecord> UserRepository::findByUsername(
    const std::string& username) const
{
    return mysql_.query(
        "SELECT id, username, password_hash FROM users WHERE username = ? LIMIT 1",
        [](sql::ResultSet& result) -> std::optional<UserRecord> {
            if (!result.next())
            {
                return std::nullopt;
            }
            return UserRecord{
                result.getInt("id"),
                result.getString("username"),
                result.getString("password_hash")
            };
        },
        username);
}

std::optional<int> UserRepository::create(
    const std::string& username,
    const std::string& passwordHash) const
{
    try
    {
        const long long id = mysql_.executeInsert(
            "INSERT INTO users (username, password_hash) VALUES (?, ?)",
            username,
            passwordHash);
        return static_cast<int>(id);
    }
    catch (const http::db::DbException& e)
    {
        if (e.errorCode() == 1062)
        {
            return std::nullopt;
        }
        throw;
    }
}
