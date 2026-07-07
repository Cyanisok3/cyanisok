 #pragma once
 #include "db/DbConnectionPool.h"
 
#include <string>

namespace http
{

class MysqlUtil
{
public:
    static void init(const std::string& host, const std::string& user,
                    const std::string& password, const std::string& database,
                    size_t poolSize = 10)
    {
        http::db::DbConnectionPool::getInstance().init(
            host, user, password, database, poolSize);
    }

    template<typename Callback, typename... Args>
    auto query(const std::string& sql, Callback&& callback, Args&&... args)
        -> decltype(callback(std::declval<sql::ResultSet&>()))
    {
        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        return conn->query(
            sql,
            std::forward<Callback>(callback),
            std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    int executeUpdate(const std::string& sql, Args&&... args)
    {
        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        return conn->executeUpdate(sql, std::forward<Args>(args)...);
    }

    template<typename... Args>
    long long executeInsert(const std::string& sql, Args&&... args)
    {
        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        return conn->executeInsert(sql, std::forward<Args>(args)...);
    }

    bool ping()
    {
        auto conn = http::db::DbConnectionPool::getInstance().getConnection();
        return conn->ping();
    }
};

} // namespace http
