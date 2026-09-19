/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#include "ThreadPool.h"

bool ThreadPool::hasPendingTaskLocked() const {
    for (const auto& queue : tasksByPriority_) {
        if (!queue.empty()) {
            return true;
        }
    }
    return false;
}

std::function<void()> ThreadPool::popNextTaskLocked() {
    // kNumPriorityLevels - 1 is the highest priority; scan downward so a
    // worker always prefers the highest-priority non-empty queue.
    for (size_t i = kNumPriorityLevels; i-- > 0;) {
        auto& queue = tasksByPriority_[i];
        if (!queue.empty()) {
            std::function<void()> task = std::move(queue.front());
            queue.pop();
            return task;
        }
    }
    // Unreachable if the caller checked hasPendingTaskLocked() first.
    return std::function<void()>();
}

/**
 * @brief Constructor for ThreadPool that initializes worker threads.
 *
 * @param[in] numThreads Number of threads to create in the pool.
 * @param[in] logger A shared pointer to a logger instance for logging messages.
 */
ThreadPool::ThreadPool(size_t numThreads, std::shared_ptr<ILogger> logger)
    : stop_(false), logger_(logger) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back(
            [this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex_);
                        this->condition_.wait(lock,
                            [this] { return this->stop_ || this->hasPendingTaskLocked(); });
                        if (this->stop_ && !this->hasPendingTaskLocked())
                            return;
                        task = this->popNextTaskLocked();
                    }
                    if (logger_) {
                        logger_->info("ThreadPool: Executing task.");
                    }
                    task();
                }
            }
        );
        if (logger_) {
            logger_->info("ThreadPool: Created worker thread " + std::to_string(i));
        }
    }
}

/**
 * @brief Destructor for ThreadPool that ensures proper shutdown of worker threads.
 */
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
            if (logger_) {
                logger_->info("ThreadPool: Worker thread joined.");
            }
        }
    }
}
