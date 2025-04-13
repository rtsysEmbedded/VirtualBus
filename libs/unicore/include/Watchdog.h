/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#include "VirtualBus.h"
#include "ErrorHandler.h"
#include "ILogger.h"

/**
 * @brief Constructor for VirtualBus that initializes the bus as running and creates a thread pool.
 *
 * @param[in] logger A shared pointer to a logger instance for logging messages.
 */
VirtualBus::VirtualBus(std::shared_ptr<ILogger> logger)
    : running_(true), threadPool_(std::thread::hardware_concurrency(), logger), logger_(logger) {
    if (logger_) {
        logger_->info("VirtualBus: Initialized with " + std::to_string(std::thread::hardware_concurrency()) + " worker threads.");
    }
}

/**
 * @brief Destructor for VirtualBus that shuts down the bus.
 */
VirtualBus::~VirtualBus() {
    shutdown();
    if (logger_) {
        logger_->info("VirtualBus: Shut down.");
    }
}

/**
 * @brief Attaches a task to the virtual bus.
 *
 * @param[in] taskId The identifier of the task.
 * @param[in] taskName The name of the task.
 */
ReturnType VirtualBus::attach(int taskId, const std::string& taskName) {
    std::lock_guard<std::mutex> lock(busMutex_);
    if (tasks_.find(taskId) != tasks_.end()) {
        if (logger_) {
            logger_->warn("VirtualBus: Task ID " + std::to_string(taskId) + " already exists.");
        }
        ErrorHandler::handleError("VirtualBus", "Task ID already exists.", ErrorHandler::ErrorSeverity::WARNING);
        return ReturnType::INVALID_ARGUMENT;
    }
    tasks_[taskId] = TaskInfo{taskName, std::queue<std::shared_ptr<VirtualBusCmd>>(), nullptr};
    if (logger_) {
        logger_->info("VirtualBus: Task " + taskName + " (ID: " + std::to_string(taskId) + ") attached to the bus.");
    }
    return ReturnType::OK;
}

/**
 * @brief Detaches a task from the virtual bus.
 *
 * @param[in] taskId The identifier of the task to be detached.
 */
void VirtualBus::detach(int taskId) {
    std::lock_guard<std::mutex> lock(busMutex_);
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        std::string taskName = it->second.name;
        tasks_.erase(it);
        if (logger_) {
            logger_->info("VirtualBus: Task " + taskName + " (ID: " + std::to_string(taskId) + ") detached from the bus.");
        }
    }
}

/**
 * @brief Registers a callback function for a specific task.
 *
 * @param[in] taskId The identifier of the task.
 * @param[in] callback The callback function to be registered.
 */
void VirtualBus::registerCallback(int taskId, CallbackFunction callback) {
    std::lock_guard<std::mutex> lock(busMutex_);
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        it->second.callback = callback;
        if (logger_) {
            logger_->info("VirtualBus: Callback registered for task ID " + std::to_string(taskId));
        }
    }
}

/**
 * @brief Sends a message from a sender to the virtual bus.
 *
 * @param[in] senderId The identifier of the sender.
 * @param[in] message The message to be sent.
 */
void VirtualBus::sendMessage(int senderId, const std::shared_ptr<VirtualBusCmd>& message) {
    std::vector<std::function<void()>> callbacksToInvoke;

    {
        std::lock_guard<std::mutex> lock(busMutex_);
        auto senderIt = tasks_.find(senderId);
        std::string senderName = (senderIt != tasks_.end()) ? senderIt->second.name : "Unknown";

        if (logger_) {
            logger_->info("VirtualBus: Task " + senderName + " (ID: " + std::to_string(senderId) + ") is sending a message.");
        }

        for (auto& [taskId, taskInfo] : tasks_) {
            if (taskId != senderId) {
                taskInfo.messageQueue.push(message);

                // Collect callbacks to invoke
                if (taskInfo.callback) {
                    auto callback = taskInfo.callback;
                    auto msg = message;
                    callbacksToInvoke.push_back([callback, msg]() {
                        callback(msg);
                    });
                }
            }
        }
    }

    busConditionVariable_.notify_all();

    // Enqueue callbacks to the thread pool
    for (auto& func : callbacksToInvoke) {
        threadPool_.enqueue(func);
    }
}

/**
 * @brief Receives a message for a specific task from the virtual bus.
 *
 * @param[in] taskId The identifier of the task.
 * @param[out] message The message received by the task.
 * @return True if a message is received, otherwise false.
 */
bool VirtualBus::receiveMessage(int taskId, std::shared_ptr<VirtualBusCmd>& message) {
    std::unique_lock<std::mutex> lock(busMutex_);
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        if (logger_) {
            logger_->warn("VirtualBus: Task ID " + std::to_string(taskId) + " not found.");
        }
        return false; // Task not found
    }

    auto& taskInfo = it->second;
    auto& queue = taskInfo.messageQueue;

    busConditionVariable_.wait(lock, [&queue, this] { return !queue.empty() || !running_; });

    if (!running_) {
        if (logger_) {
            logger_->info("VirtualBus: Bus is no longer running.");
        }
        return false;
    }

    if (!queue.empty()) {
        message = queue.front();
        queue.pop();
        if (logger_) {
            logger_->info("VirtualBus: Message received for task ID " + std::to_string(taskId));
        }
        return true;
    }
    return false;
}

/**
 * @brief Shuts down the virtual bus.
 */
void VirtualBus::shutdown() {
    {
        std::lock_guard<std::mutex> lock(busMutex_);
        running_ = false;
    }
    busConditionVariable_.notify_all();
    if (logger_) {
        logger_->info("VirtualBus: Shutting down.");
    }
}
