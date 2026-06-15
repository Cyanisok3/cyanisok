#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class BoundedExecutor
{
public:
    BoundedExecutor(std::size_t workerCount, std::size_t queueCapacity);
    ~BoundedExecutor();

    BoundedExecutor(const BoundedExecutor&) = delete;
    BoundedExecutor& operator=(const BoundedExecutor&) = delete;

    bool trySubmit(std::function<void()> task);
    void shutdown();

private:
    void workerLoop();

    const std::size_t queueCapacity_;
    std::mutex mutex_;
    std::condition_variable workAvailable_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
};
