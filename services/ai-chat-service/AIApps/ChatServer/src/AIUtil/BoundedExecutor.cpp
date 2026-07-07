#include "../../include/AIUtil/BoundedExecutor.h"

#include <stdexcept>

BoundedExecutor::BoundedExecutor(
    std::size_t workerCount,
    std::size_t queueCapacity)
    : queueCapacity_(queueCapacity)
{
    if (workerCount == 0 || queueCapacity == 0)
    {
        throw std::invalid_argument("Executor worker and queue sizes must be positive");
    }

    workers_.reserve(workerCount);
    for (std::size_t i = 0; i < workerCount; ++i)
    {
        workers_.emplace_back(&BoundedExecutor::workerLoop, this);
    }
}

BoundedExecutor::~BoundedExecutor()
{
    shutdown();
}

bool BoundedExecutor::trySubmit(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_ || tasks_.size() >= queueCapacity_)
        {
            return false;
        }
        tasks_.push(std::move(task));
    }
    workAvailable_.notify_one();
    return true;
}

void BoundedExecutor::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_)
        {
            return;
        }
        stopping_ = true;
    }
    workAvailable_.notify_all();

    for (auto& worker : workers_)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
    workers_.clear();
}

void BoundedExecutor::workerLoop()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            workAvailable_.wait(lock, [this] {
                return stopping_ || !tasks_.empty();
            });
            if (stopping_ && tasks_.empty())
            {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        try
        {
            task();
        }
        catch (...)
        {
            // Tasks own their error reporting; keep worker threads alive.
        }
    }
}
