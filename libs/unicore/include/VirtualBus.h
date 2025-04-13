/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef VIRTUAL_BUS_H
#define VIRTUAL_BUS_H

#include <iostream>
#include <unordered_map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <atomic>

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

public:
    using CallbackFunction = std::function<void(std::shared_ptr<VirtualBusCmd>)>;

    /**
     * @brief Constructor for VirtualBus.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    VirtualBus(std::shared_ptr<ILogger> logger = nullptr);

    /**
     * @brief Destructor for VirtualBus.
     */
    ~VirtualBus();

    /**
     * @brief Attaches a task to the virtual bus.
     *
     * @param[in] taskId The identifier of the task.
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
     * @param[in] senderId The identifier of the sender.
     * @param[in] message The message to be sent.
     */
    void sendMessage(int senderId, const std::shared_ptr<VirtualBusCmd>& message);

    /**
     * @brief Receives a message for a specific task from the virtual bus.
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
        std::queue<std::shared_ptr<VirtualBusCmd>> messageQueue;  ///< Queue of messages for the task
        CallbackFunction callback;  ///< Callback function for the task
    };

    std::unordered_map<int, TaskInfo> tasks_;  ///< Map of tasks registered with the virtual bus
    std::mutex busMutex_;  ///< Mutex for synchronizing access to the bus
    std::condition_variable busConditionVariable_;  ///< Condition variable for message synchronization
    std::atomic<bool> running_;  ///< Atomic flag indicating whether the bus is running

    ThreadPool threadPool_;  ///< Thread pool for handling tasks
};

#endif // VIRTUAL_BUS_H
