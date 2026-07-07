#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_driver.h>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include <type_traits>
#include <utility>
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

    template<typename Callback, typename... Args>
    auto query(const std::string& sql, Callback&& callback, Args&&... args)
        -> decltype(callback(std::declval<sql::ResultSet&>()))
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql)
            );
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            std::unique_ptr<sql::ResultSet> result(stmt->executeQuery());
            return callback(*result);
        }
        catch (const sql::SQLException& e)
        {
            LOG_ERROR << "Query failed: " << e.what() << ", SQL: " << sql;
            throw DbException(e.what(), e.getErrorCode());
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
            throw DbException(e.what(), e.getErrorCode());
        }
    }

    template<typename... Args>
    long long executeInsert(const std::string& sql, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            std::unique_ptr<sql::PreparedStatement> stmt(
                conn_->prepareStatement(sql)
            );
            bindParams(stmt.get(), 1, std::forward<Args>(args)...);
            stmt->executeUpdate();

            std::unique_ptr<sql::Statement> idStmt(conn_->createStatement());
            std::unique_ptr<sql::ResultSet> result(
                idStmt->executeQuery("SELECT LAST_INSERT_ID()")
            );
            if (!result->next())
            {
                throw DbException("Insert succeeded but no generated id was returned");
            }
            return result->getInt64(1);
        }
        catch (const sql::SQLException& e)
        {
            LOG_ERROR << "Insert failed: " << e.what() << ", SQL: " << sql;
            throw DbException(e.what(), e.getErrorCode());
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
        else if constexpr (std::is_same_v<ValueType, bool>)
        {
            stmt->setBoolean(index, value);
        }
        else if constexpr (std::is_integral_v<ValueType>)
        {
            stmt->setInt64(index, static_cast<int64_t>(value));
        }
        else if constexpr (std::is_floating_point_v<ValueType>)
        {
            stmt->setDouble(index, static_cast<double>(value));
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
