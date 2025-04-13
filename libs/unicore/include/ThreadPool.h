/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include "ILogger.h"
#include "ErrorHandler.h"
#include <memory>

/**
 * @brief Class representing a thread pool for executing tasks concurrently.
 */
class ThreadPool {
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Constructor to initialize the thread pool with the specified number of threads.
     *
     * @param[in] numThreads Number of threads to be created in the pool.
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    explicit ThreadPool(size_t numThreads, std::shared_ptr<ILogger> logger = nullptr);

    /**
     * @brief Destructor to properly shut down the thread pool.
     */
    ~ThreadPool() ;

    /**
     * @brief Adds a new task to the pool.
     *
     * @tparam F Function type.
     * @tparam Args Argument types.
     * @param[in] f Function to be executed.
     * @param[in] args Arguments to be passed to the function.
     * @return A future representing the result of the task.
     */
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
            -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::vector<std::thread> workers_;  ///< Vector containing worker threads
    std::queue<std::function<void()>> tasks_;  ///< Queue of tasks to be executed

    std::mutex queueMutex_;  ///< Mutex for synchronizing access to the task queue
    std::condition_variable condition_;  ///< Condition variable to notify worker threads
    std::atomic<bool> stop_;  ///< Atomic flag to indicate if the pool should stop
};

// Implementation of template methods

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {

        using ReturnType = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            // Don't allow enqueueing after stopping the pool
            if (stop_) {
                if (logger_) {
                    logger_->error("ThreadPool: Attempted to enqueue on stopped ThreadPool.");
                }
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        if (logger_) {
            logger_->info("ThreadPool: Task enqueued.");
        }
        return result;
}



#endif // THREAD_POOL_H
