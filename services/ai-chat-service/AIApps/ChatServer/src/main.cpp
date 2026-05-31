#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include "../include/ChatServer.h"

// Get environment variable with default fallback
const std::string getEnvOrDefault(const char* key, const char* defaultVal) {
    const char* val = std::getenv(key);
    return val ? std::string(val) : std::string(defaultVal);
}

const std::string RABBITMQ_HOST = getEnvOrDefault("RABBITMQ_HOST", "rabbitmq");
const std::string RABBITMQ_PORT = getEnvOrDefault("RABBITMQ_PORT", "5672");
const std::string RABBITMQ_USER = getEnvOrDefault("RABBITMQ_USER", "guest");
const std::string RABBITMQ_PASS = getEnvOrDefault("RABBITMQ_PASS", "guest");
const std::string QUEUE_NAME = "sql_queue";
const int THREAD_NUM = 2;

// Persist structured messages from RabbitMQ with prepared statements.
void executeMysql(const std::string payload) {
    try {
        json message = json::parse(payload);
        if (message.value("type", "") != "insert_chat_message") {
            LOG_WARN << "Ignoring unknown RabbitMQ payload: " << payload;
            return;
        }

        http::MysqlUtil mysqlUtil_;
        mysqlUtil_.executeUpdate(
            "INSERT INTO chat_message (user_id, username, is_user, content, ts) VALUES (?, ?, ?, ?, ?)",
            message.value("user_id", 0),
            message.value("username", ""),
            message.value("is_user", false) ? 1 : 0,
            message.value("content", ""),
            message.value("ts", 0LL));
    }
    catch (const std::exception& e) {
        LOG_ERROR << "Failed to persist RabbitMQ payload: " << e.what();
    }
}

int main(int argc, char* argv[]) {
    LOG_INFO << "pid = " << getpid();
    std::string serverName = "ChatServer";
    int port = 80;

    // Parse command line options
    int opt;
    const char* str = "p:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }

    muduo::Logger::setLogLevel(muduo::Logger::WARN);
    ChatServer server(port, serverName);
    server.setThreadNum(4);

    // Wait for MySQL to be ready
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Load chat history from MySQL
    server.initChatMessage();

    // Start RabbitMQ consumer thread pool
    RabbitMQThreadPool pool(RABBITMQ_HOST, QUEUE_NAME, THREAD_NUM, executeMysql, RABBITMQ_USER, RABBITMQ_PASS);
    pool.start();

    server.start();
}
