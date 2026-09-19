/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <array>
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
 *
 * Tasks are dispatched from one of kNumPriorityLevels FIFO queues rather
 * than a single queue: a worker always prefers the highest-priority
 * non-empty queue, so a burst of low-priority work can't delay a
 * higher-priority task behind it, while FIFO order within a single
 * priority level stays deterministic (unlike a comparator-driven
 * std::priority_queue, which doesn't preserve arrival order between
 * equal-priority elements).
 *
 * Deliberately decoupled from VirtualBusCmd::Priority (this is a
 * general-purpose utility, not VirtualBus-specific): callers pass a raw
 * priority index instead. By convention 0 is the lowest priority and
 * kNumPriorityLevels - 1 is the highest, matching VirtualBusCmd::Priority's
 * own Low=0..Critical=3 ordering, so VirtualBus can pass
 * static_cast<size_t>(message->getPriority()) straight through with no
 * translation.
 */
class ThreadPool {
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /// Number of priority levels. Must match VirtualBusCmd::kPriorityLevels
    /// for VirtualBus's direct static_cast<size_t>(Priority) passthrough to
    /// stay in range.
    static constexpr size_t kNumPriorityLevels = 4;

    /// Convenience default for callers that don't care about priority.
    static constexpr size_t kDefaultPriority = 1; // matches Priority::Normal

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
     * @brief Adds a new task to the pool at a given priority.
     *
     * @tparam F Function type.
     * @tparam Args Argument types.
     * @param[in] priority Priority index in [0, kNumPriorityLevels); higher
     * runs first. Out-of-range values are clamped into range.
     * @param[in] f Function to be executed.
     * @param[in] args Arguments to be passed to the function.
     * @return A future representing the result of the task.
     */
    template<class F, class... Args>
    auto enqueue(size_t priority, F&& f, Args&&... args)
            -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::vector<std::thread> workers_;  ///< Vector containing worker threads
    std::array<std::queue<std::function<void()>>, kNumPriorityLevels> tasksByPriority_;  ///< One FIFO queue per priority level

    std::mutex queueMutex_;  ///< Mutex for synchronizing access to the task queues
    std::condition_variable condition_;  ///< Condition variable to notify worker threads
    std::atomic<bool> stop_;  ///< Atomic flag to indicate if the pool should stop

    /// Returns true if any priority queue is non-empty. Caller must hold queueMutex_.
    bool hasPendingTaskLocked() const;

    /// Pops and returns the next task, scanning from the highest priority
    /// queue down to the lowest. Caller must hold queueMutex_ and must have
    /// already verified hasPendingTaskLocked().
    std::function<void()> popNextTaskLocked();
};

// Implementation of template methods

template<class F, class... Args>
auto ThreadPool::enqueue(size_t priority, F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {

        using TaskReturnType = typename std::result_of<F(Args...)>::type;

        if (priority >= kNumPriorityLevels) {
            priority = kNumPriorityLevels - 1;
        }

        auto task = std::make_shared<std::packaged_task<TaskReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<TaskReturnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            // Don't allow enqueueing after stopping the pool
            if (stop_) {
                if (logger_) {
                    logger_->error("ThreadPool: Attempted to enqueue on stopped ThreadPool.");
                }
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasksByPriority_[priority].emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        if (logger_) {
            logger_->info("ThreadPool: Task enqueued at priority " + std::to_string(priority) + ".");
        }
        return result;
}



#endif // THREAD_POOL_H
