/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#include "VirtualBus.h"
#include "ErrorHandler.h"
#include "SystemClock.h"

/**
 * @brief Constructor for VirtualBus that initializes the bus as running and creates a thread pool.
 *
 * @param[in] logger A shared pointer to a logger instance for logging messages.
 * @param[in] clock A shared pointer to a time source; defaults to a real-time SystemClock.
 * @param[in] maxQueueDepth Cap applied to each per-task, per-priority message queue.
 * @param[in] threadPoolQueueDepth Cap applied to the internal ThreadPool's dispatch queues.
 */
VirtualBus::VirtualBus(std::shared_ptr<ILogger> logger, std::shared_ptr<IClock> clock,
                       size_t maxQueueDepth, size_t threadPoolQueueDepth)
    : logger_(logger), clock_(clock ? std::move(clock) : std::make_shared<SystemClock>()),
      running_(true), maxQueueDepth_(maxQueueDepth),
      threadPool_(std::thread::hardware_concurrency(), nullptr, threadPoolQueueDepth) {
    if (logger_) {
        logger_->info("VirtualBus: Initialized with " + std::to_string(std::thread::hardware_concurrency()) +
                      " worker threads, max queue depth " + std::to_string(maxQueueDepth_) +
                      ", thread pool queue depth " + std::to_string(threadPoolQueueDepth) + ".");
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

bool VirtualBus::deliverToTaskLocked(int taskId, TaskInfo& taskInfo, const std::shared_ptr<VirtualBusCmd>& message,
                                      std::vector<std::function<void()>>& callbacksToInvoke) {
    size_t priorityIndex = static_cast<size_t>(message->getPriority());
    if (priorityIndex >= kPriorityLevels) {
        priorityIndex = kPriorityLevels - 1;
    }

    auto& queue = taskInfo.messageQueues[priorityIndex];
    if (queue.size() >= maxQueueDepth_) {
        // Reject-new: the new message is dropped rather than evicting an
        // older one or blocking the sender. sendMessage() surfaces this
        // as ReturnType::BUSY to whoever called it.
        if (logger_) {
            logger_->warn("VirtualBus: Task ID " + std::to_string(taskId) + "'s queue at priority " +
                          std::to_string(priorityIndex) + " is full (depth " + std::to_string(maxQueueDepth_) +
                          "); dropping message.");
        }
        return false;
    }

    queue.push(message);

    if (taskInfo.callback) {
        auto callback = taskInfo.callback;
        auto msg = message;
        callbacksToInvoke.push_back([callback, msg]() {
            callback(msg);
        });
    }
    return true;
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
    bool anyRecipientDropped = false;

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
                    if (!deliverToTaskLocked(taskId, taskInfo, message, callbacksToInvoke)) {
                        anyRecipientDropped = true;
                    }
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
            if (!deliverToTaskLocked(targetId, targetIt->second, message, callbacksToInvoke)) {
                anyRecipientDropped = true;
            }
        }
    }

    busConditionVariable_.notify_all();

    // Enqueue callbacks to the thread pool at the message's priority. The
    // message is already safely queued for polling receivers at this
    // point regardless of what happens here: a full ThreadPool dispatch
    // queue means the async callback notification couldn't be scheduled
    // right now, not that the message itself was lost.
    for (auto& func : callbacksToInvoke) {
        try {
            threadPool_.enqueue(dispatchPriority, func);
        } catch (const std::runtime_error& e) {
            anyRecipientDropped = true;
            if (logger_) {
                logger_->warn(std::string("VirtualBus: Callback dispatch queue full, notification skipped: ") + e.what());
            }
        }
    }

    return anyRecipientDropped ? ReturnType::BUSY : ReturnType::OK;
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
