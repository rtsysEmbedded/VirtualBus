/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#include "ThreadPool.h"

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
                            [this] { return this->stop_ || !this->tasks_.empty(); });
                        if (this->stop_ && this->tasks_.empty())
                            return;
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
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
