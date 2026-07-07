#pragma once

#include <optional>
#include <string>

#include "../../../../HttpServer/include/utils/MysqlUtil.h"

struct UserRecord
{
    int id;
    std::string username;
    std::string passwordHash;
};

class UserRepository
{
public:
    std::optional<UserRecord> findByUsername(const std::string& username) const;
    std::optional<int> create(
        const std::string& username,
        const std::string& passwordHash) const;

private:
    mutable http::MysqlUtil mysql_;
};
