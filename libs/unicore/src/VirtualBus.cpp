/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#include "VirtualBus.h"
#include "ErrorHandler.h"
#include "SystemClock.h"

/**
 * @brief Constructor for VirtualBus that initializes the bus as running and creates a thread pool.
 *
 * @param[in] logger A shared pointer to a logger instance for logging messages.
 * @param[in] clock A shared pointer to a time source; defaults to a real-time SystemClock.
 */
VirtualBus::VirtualBus(std::shared_ptr<ILogger> logger, std::shared_ptr<IClock> clock)
    : logger_(logger), clock_(clock ? std::move(clock) : std::make_shared<SystemClock>()),
      running_(true), threadPool_(std::thread::hardware_concurrency()) {
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
    if (taskId < 0) {
        if (logger_) {
            logger_->warn("VirtualBus: Rejected attach for negative task ID " + std::to_string(taskId) +
                          " (negative ids are reserved, e.g. kBroadcast).");
        }
        ErrorHandler::handleError("VirtualBus", "Task ID must be non-negative.", ErrorHandler::ErrorSeverity::WARNING, logger_);
        return ReturnType::INVALID_ARGUMENT;
    }
    if (tasks_.find(taskId) != tasks_.end()) {
        if (logger_) {
            logger_->warn("VirtualBus: Task ID " + std::to_string(taskId) + " already exists.");
        }
        ErrorHandler::handleError("VirtualBus", "Task ID already exists.", ErrorHandler::ErrorSeverity::WARNING, logger_);
        return ReturnType::INVALID_ARGUMENT;
    }
    tasks_[taskId].name = taskName;
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
    } else {
        if (logger_) {
            logger_->warn("VirtualBus: Attempted to detach non-existent task ID " + std::to_string(taskId));
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
    } else {
        if (logger_) {
            logger_->warn("VirtualBus: Attempted to register callback for non-existent task ID " + std::to_string(taskId));
        }
    }
}

void VirtualBus::deliverToTaskLocked(TaskInfo& taskInfo, const std::shared_ptr<VirtualBusCmd>& message,
                                      std::vector<std::function<void()>>& callbacksToInvoke) {
    size_t priorityIndex = static_cast<size_t>(message->getPriority());
    if (priorityIndex >= kPriorityLevels) {
        priorityIndex = kPriorityLevels - 1;
    }
    taskInfo.messageQueues[priorityIndex].push(message);

    if (taskInfo.callback) {
        auto callback = taskInfo.callback;
        auto msg = message;
        callbacksToInvoke.push_back([callback, msg]() {
            callback(msg);
        });
    }
}

/**
 * @brief Sends a message from a sender to the virtual bus.
 *
 * @param[in] senderId The identifier of the sender.
 * @param[in] message The message to be sent.
 * @param[in] targetId kBroadcast, or a specific task id.
 */
ReturnType VirtualBus::sendMessage(int senderId, const std::shared_ptr<VirtualBusCmd>& message, int targetId) {
    std::vector<std::function<void()>> callbacksToInvoke;
    // Priority passed to ThreadPool::enqueue for the callbacks below.
    // Read once, outside the lock: getPriority()/getType() etc. on a
    // shared_ptr<VirtualBusCmd> the caller still owns is the caller's
    // responsibility to keep stable across this call, same as message's
    // contents already were before this change.
    size_t dispatchPriority = static_cast<size_t>(message->getPriority());

    {
        std::lock_guard<std::mutex> lock(busMutex_);
        auto senderIt = tasks_.find(senderId);
        std::string senderName = (senderIt != tasks_.end()) ? senderIt->second.name : "Unknown";

        if (logger_) {
            logger_->info("VirtualBus: Task " + senderName + " (ID: " + std::to_string(senderId) + ") is sending a message.");
        }

        if (senderIt == tasks_.end()) {
            ErrorHandler::handleError("VirtualBus", "Sender task ID " + std::to_string(senderId) + " not found.", ErrorHandler::ErrorSeverity::WARNING, logger_);
            return ReturnType::NOT_FOUND;
        }

        message->updateTimestamp(clock_->nowMs());

        if (targetId == kBroadcast) {
            for (auto& [taskId, taskInfo] : tasks_) {
                if (taskId != senderId) {
                    deliverToTaskLocked(taskInfo, message, callbacksToInvoke);
                }
            }
        } else if (targetId == senderId) {
            if (logger_) {
                logger_->warn("VirtualBus: Task " + std::to_string(senderId) + " attempted to send a targeted message to itself.");
            }
            return ReturnType::INVALID_ARGUMENT;
        } else {
            auto targetIt = tasks_.find(targetId);
            if (targetIt == tasks_.end()) {
                if (logger_) {
                    logger_->warn("VirtualBus: Target task ID " + std::to_string(targetId) + " not found.");
                }
                ErrorHandler::handleError("VirtualBus", "Target task ID " + std::to_string(targetId) + " not found.", ErrorHandler::ErrorSeverity::WARNING, logger_);
                return ReturnType::NOT_FOUND;
            }
            deliverToTaskLocked(targetIt->second, message, callbacksToInvoke);
        }
    }

    busConditionVariable_.notify_all();

    // Enqueue callbacks to the thread pool at the message's priority.
    for (auto& func : callbacksToInvoke) {
        threadPool_.enqueue(dispatchPriority, func);
    }

    return ReturnType::OK;
}

/**
 * @brief Receives the highest-priority pending message for a specific task from the virtual bus.
 *
 * @param[in] taskId The identifier of the task.
 * @param[out] message The message received by the task.
 * @return True if a message is received, otherwise false.
 */
bool VirtualBus::receiveMessage(int taskId, std::shared_ptr<VirtualBusCmd>& message) {
    std::unique_lock<std::mutex> lock(busMutex_);
    if (tasks_.find(taskId) == tasks_.end()) {
        if (logger_) {
            logger_->warn("VirtualBus: Task ID " + std::to_string(taskId) + " not found.");
        }
        return false; // Task not found
    }

    // wait() releases busMutex_ while parked, so a concurrent detach() can
    // erase this task's entry (and its queues) out from under us at any
    // point before we reacquire the lock. Re-look-up the task by id on
    // every predicate check instead of capturing a reference across the
    // wait (see the history of this function for the bug that pattern
    // caused).
    busConditionVariable_.wait(lock, [this, taskId] {
        auto it = tasks_.find(taskId);
        if (it == tasks_.end() || !running_) {
            return true;
        }
        for (const auto& queue : it->second.messageQueues) {
            if (!queue.empty()) {
                return true;
            }
        }
        return false;
    });

    if (!running_) {
        if (logger_) {
            logger_->info("VirtualBus: Bus is no longer running.");
        }
        return false;
    }

    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        if (logger_) {
            logger_->warn("VirtualBus: Task ID " + std::to_string(taskId) + " was detached while waiting for a message.");
        }
        return false;
    }

    // Highest priority first (see TaskInfo::messageQueues's doc comment).
    for (size_t p = kPriorityLevels; p-- > 0;) {
        auto& queue = it->second.messageQueues[p];
        if (!queue.empty()) {
            message = queue.front();
            queue.pop();
            if (logger_) {
                logger_->info("VirtualBus: Message received for task ID " + std::to_string(taskId) +
                              " at priority " + std::to_string(p));
            }
            return true;
        }
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
    ErrorHandler::handleError("VirtualBus", "Bus is shutting down.", ErrorHandler::ErrorSeverity::INFO, logger_);
}
