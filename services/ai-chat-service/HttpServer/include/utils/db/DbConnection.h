#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include <type_traits>
#include "DbException.h"

namespace http
{
namespace db
{

class DbConnection
{
public:
    DbConnection(const std::string& host,
                const std::string& user,
                const std::string& password,
                const std::string& database);
    ~DbConnection();

    DbConnection(const DbConnection&) = delete;
    DbConnection& operator=(const DbConnection&) = delete;

    bool isValid();
    void reconnect();
    void cleanup();

    template<typename... Args>
    sql::ResultSet* executeQuery(const std::string& sql, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql)
            );
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            return stmt->executeQuery();
        }
        catch (const sql::SQLException& e)
        {
            LOG_ERROR << "Query failed: " << e.what() << ", SQL: " << sql;
            throw DbException(e.what());
        }
    }

    template<typename... Args>
    int executeUpdate(const std::string& sql, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql)
            );
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            return stmt->executeUpdate();
        }
        catch (const sql::SQLException& e)
        {
            LOG_ERROR << "Update failed: " << e.what() << ", SQL: " << sql;
            throw DbException(e.what());
        }
    }

    bool ping();

private:
    void bindParams(sql::PreparedStatement*, int) {}

    template<typename T, typename... Args>
    void bindParams(sql::PreparedStatement* stmt, int index,
                   T&& value, Args&&... args)
    {
        bindOne(stmt, index, std::forward<T>(value));
        bindParams(stmt, index + 1, std::forward<Args>(args)...);
    }

    template<typename T>
    void bindOne(sql::PreparedStatement* stmt, int index, T&& value)
    {
        using ValueType = std::decay_t<T>;

        if constexpr (std::is_same_v<ValueType, std::string>)
        {
            stmt->setString(index, value);
        }
        else if constexpr (std::is_convertible_v<T, std::string>)
        {
            stmt->setString(index, std::string(std::forward<T>(value)));
        }
        else if constexpr (std::is_arithmetic_v<ValueType>)
        {
            stmt->setString(index, std::to_string(value));
        }
        else
        {
            static_assert(std::is_convertible_v<T, std::string>, "Unsupported SQL parameter type");
        }
    }

private:
    std::shared_ptr<sql::Connection> conn_;
    std::string                      host_;
    std::string                      user_;
    std::string                      password_;
    std::string                      database_;
    std::mutex                       mutex_;
};

} // namespace db
} // namespace http
