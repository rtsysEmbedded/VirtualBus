/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef VIRTUAL_BUS_H
#define VIRTUAL_BUS_H

#include <array>
#include <iostream>
#include <unordered_map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <atomic>

#include "IClock.h"
#include "ThreadPool.h"
#include "VirtualBusCmd.h"
#include "ReturnType.h"
#include "ILogger.h"

/**
 * @brief Class representing a virtual communication bus.
 */
class VirtualBus {
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages
    std::shared_ptr<IClock> clock_; ///< Time source used to stamp sent messages

public:
    using CallbackFunction = std::function<void(std::shared_ptr<VirtualBusCmd>)>;

    /// Sentinel targetId meaning "broadcast to every attached task except
    /// the sender" -- sendMessage()'s default, matching the bus's original
    /// (and only) behavior before targeted delivery existed. Negative, so
    /// it can never collide with a real task id (attach() rejects negative
    /// ids -- see attach()'s doc comment).
    static constexpr int kBroadcast = -1;

    /**
     * @brief Constructor for VirtualBus.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     * @param[in] clock A shared pointer to a time source for stamping sent
     * messages. Defaults to a real-time SystemClock; inject a VirtualClock
     * for deterministic timestamps in tests/benchmarks (and, eventually, a
     * record/replay engine).
     */
    VirtualBus(std::shared_ptr<ILogger> logger = nullptr, std::shared_ptr<IClock> clock = nullptr);

    /**
     * @brief Destructor for VirtualBus.
     */
    ~VirtualBus();

    /**
     * @brief Attaches a task to the virtual bus.
     *
     * @param[in] taskId The identifier of the task. Must be non-negative:
     * negative ids are reserved (see kBroadcast) and rejected with
     * ReturnType::INVALID_ARGUMENT.
     * @param[in] taskName The name of the task.
     */
    ReturnType attach(int taskId, const std::string& taskName);

    /**
     * @brief Detaches a task from the virtual bus.
     *
     * @param[in] taskId The identifier of the task to be detached.
     */
    void detach(int taskId);

    /**
     * @brief Registers a callback function for a specific task.
     *
     * @param[in] taskId The identifier of the task.
     * @param[in] callback The callback function to be registered.
     */
    void registerCallback(int taskId, CallbackFunction callback);

    /**
     * @brief Sends a message from a sender to the virtual bus.
     *
     * Stamps the message's timestamp (via updateTimestamp()) using this
     * bus's IClock before delivering it, so the timestamp a receiver sees
     * reflects when the message entered the bus rather than when it was
     * constructed.
     *
     * @param[in] senderId The identifier of the sender.
     * @param[in] message The message to be sent.
     * @param[in] targetId Either kBroadcast (default: deliver to every
     * other attached task, the bus's original behavior) or a specific
     * task id to deliver only to that task.
     * @return ReturnType::OK on success; ReturnType::NOT_FOUND if senderId
     * or (for a targeted send) targetId isn't attached;
     * ReturnType::INVALID_ARGUMENT if targetId == senderId.
     */
    ReturnType sendMessage(int senderId, const std::shared_ptr<VirtualBusCmd>& message, int targetId = kBroadcast);

    /**
     * @brief Receives the highest-priority pending message for a specific
     * task from the virtual bus, blocking until one is available.
     *
     * @param[in] taskId The identifier of the task.
     * @param[out] message The message received by the task.
     * @return True if a message is received, otherwise false.
     */
    bool receiveMessage(int taskId, std::shared_ptr<VirtualBusCmd>& message);

    /**
     * @brief Shuts down the virtual bus.
     */
    void shutdown();

private:
    /**
     * @brief Struct representing information about a task.
     */
    struct TaskInfo {
        std::string name;  ///< The name of the task
        std::array<std::queue<std::shared_ptr<VirtualBusCmd>>, kPriorityLevels> messageQueues;  ///< One FIFO queue per Priority level
        CallbackFunction callback;  ///< Callback function for the task
    };

    /// Delivers `message` into `taskInfo`'s queue for its priority and, if
    /// a callback is registered, appends an invocation of it to
    /// `callbacksToInvoke`. Caller must hold busMutex_.
    void deliverToTaskLocked(TaskInfo& taskInfo, const std::shared_ptr<VirtualBusCmd>& message,
                              std::vector<std::function<void()>>& callbacksToInvoke);

    std::unordered_map<int, TaskInfo> tasks_;  ///< Map of tasks registered with the virtual bus
    std::mutex busMutex_;  ///< Mutex for synchronizing access to the bus
    std::condition_variable busConditionVariable_;  ///< Condition variable for message synchronization
    std::atomic<bool> running_;  ///< Atomic flag indicating whether the bus is running

    ThreadPool threadPool_;  ///< Thread pool for handling tasks
};

#endif // VIRTUAL_BUS_H
