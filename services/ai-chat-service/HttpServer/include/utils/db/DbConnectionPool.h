#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include "DbConnection.h"

namespace http
{
namespace db
{

class DbConnectionPool
{
public:
    static DbConnectionPool& getInstance()
    {
        static DbConnectionPool instance;
        return instance;
    }

    void init(const std::string& host,
             const std::string& user,
             const std::string& password,
             const std::string& database,
             size_t poolSize = 10);

    std::shared_ptr<DbConnection> getConnection(
        std::chrono::milliseconds timeout = std::chrono::seconds(5));

private:
    DbConnectionPool();
    ~DbConnectionPool();

    DbConnectionPool(const DbConnectionPool&) = delete;
    DbConnectionPool& operator=(const DbConnectionPool&) = delete;

    std::shared_ptr<DbConnection> createConnection();

private:
    std::string                               host_;
    std::string                               user_;
    std::string                               password_;
    std::string                               database_;
    std::queue<std::shared_ptr<DbConnection>> connections_;
    std::mutex                                mutex_;
    std::condition_variable                   cv_;
    bool                                      initialized_ = false;
};

} // namespace db
} // namespace http
