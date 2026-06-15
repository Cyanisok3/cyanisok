#include <cstdlib>
#include <string>

#include <curl/curl.h>
#include <muduo/base/Logging.h>
#include <muduo/net/TcpServer.h>

#include "../include/ChatServer.h"

namespace
{

int positiveEnv(const char* key, int fallback)
{
    const char* raw = std::getenv(key);
    if (!raw || raw[0] == '\0')
    {
        return fallback;
    }
    try
    {
        const int value = std::stoi(raw);
        return value > 0 ? value : fallback;
    }
    catch (...)
    {
        return fallback;
    }
}

} // namespace

int main(int argc, char* argv[])
{
    muduo::Logger::setLogLevel(muduo::Logger::WARN);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    int port = positiveEnv("HTTP_PORT", 80);
    int option = 0;
    while ((option = getopt(argc, argv, "p:")) != -1)
    {
        if (option == 'p')
        {
            port = std::atoi(optarg);
        }
    }

    ChatServer server(port, "ChatServer");
    server.setThreadNum(positiveEnv("HTTP_THREAD_COUNT", 4));
    server.start();

    curl_global_cleanup();
    return 0;
}
