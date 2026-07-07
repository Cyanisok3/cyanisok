#include "../../../include/utils/db/DbConnectionPool.h"
#include "../../../include/utils/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http
{
namespace db
{

void DbConnectionPool::init(const std::string& host,
                          const std::string& user,
                          const std::string& password,
                          const std::string& database,
                          size_t poolSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_)
    {
        return;
    }

    host_ = host;
    user_ = user;
    password_ = password;
    database_ = database;

    for (size_t i = 0; i < poolSize; ++i)
    {
        connections_.push(createConnection());
    }

    initialized_ = true;
    LOG_INFO << "Database connection pool initialized with " << poolSize << " connections";
}

DbConnectionPool::DbConnectionPool() = default;

DbConnectionPool::~DbConnectionPool()
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty())
    {
        connections_.pop();
    }
    LOG_INFO << "Database connection pool destroyed";
}

std::shared_ptr<DbConnection> DbConnectionPool::getConnection(
    std::chrono::milliseconds timeout)
{
    std::shared_ptr<DbConnection> conn;
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!initialized_)
        {
            throw DbException("Connection pool not initialized");
        }

        if (!cv_.wait_for(lock, timeout, [this] {
                return !connections_.empty();
            }))
        {
            throw DbException("Timed out waiting for a database connection");
        }

        conn = connections_.front();
        connections_.pop();
    }

    try
    {
        if (!conn->ping())
        {
            LOG_WARN << "Connection lost, attempting to reconnect...";
            conn->reconnect();
        }

        return std::shared_ptr<DbConnection>(conn.get(),
            [this, conn](DbConnection*) {
                std::lock_guard<std::mutex> lock(mutex_);
                connections_.push(conn);
                cv_.notify_one();
            });
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Failed to get connection: " << e.what();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connections_.push(conn);
            cv_.notify_one();
        }
        throw;
    }
}

std::shared_ptr<DbConnection> DbConnectionPool::createConnection()
{
    return std::make_shared<DbConnection>(host_, user_, password_, database_);
}

} // namespace db
} // namespace http
