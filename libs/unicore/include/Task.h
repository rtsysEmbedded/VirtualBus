/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef TASK_H
#define TASK_H

#include <chrono>
#include <string>
#include <thread>
#include <atomic>
#include "VirtualBus.h"
#include "ILogger.h"
#include "ErrorHandler.h"
#include "Watchdog.h"
#include <memory>

/**
 * @brief Class representing a unique task identifier generator.
 */
class TaskID {
public:
    /**
     * @brief Generates a new unique task identifier.
     * @return A unique task identifier.
     *
     * The counter is a function-local static rather than a header-defined
     * `static std::atomic<int> lastID_;` member. A member defined directly
     * in the header is a non-inline definition: it links fine as long as
     * Task.h is #included into only one .cpp file, but a second .cpp that
     * also includes it (e.g. any translation unit beyond the one pulling
     * in SendTask.h/ReciveTask.h) causes a "multiple definition" link
     * error. A function-local static inside this implicitly-inline member
     * function is guaranteed by the standard to be a single instance
     * across all translation units.
     */
    static int getID() {
        static std::atomic<int> lastID_{0};
        return lastID_++;
    }
};

/**
 * @brief Abstract base class representing a generic task.
 */
class Task {
public:
    /**
     * @brief Constructor to initialize a task with a name and attach it to the virtual bus.
     *
     * @param[in] name The name of the task.
     * @param[in] bus Reference to the virtual bus the task will use.
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     * @param[in] watchdog Optional watchdog to register with on start() and
     * feed via kickWatchdog(). Left null (the default), the task is not
     * monitored at all -- every existing call site is unaffected.
     * @param[in] watchdogTimeout How long this task may go without calling
     * kickWatchdog() before the watchdog reports it. Only meaningful when
     * `watchdog` is non-null.
     */
    Task(const std::string& name, VirtualBus& bus, std::shared_ptr<ILogger> logger = nullptr,
         std::shared_ptr<Watchdog> watchdog = nullptr,
         std::chrono::milliseconds watchdogTimeout = std::chrono::milliseconds(5000))
        : name_(name), bus_(bus), running_(false), logger_(logger),
          watchdog_(std::move(watchdog)), watchdogTimeout_(watchdogTimeout) {
            id_ = TaskID::getID();
            if (logger_) {
                logger_->info("Task: Initialized task " + name_ + " with ID " + std::to_string(id_));
            }
        }

    /**
     * @brief Destructor to stop the task and detach it from the virtual bus.
     */
    virtual ~Task() {
        stop();
        if (logger_) {
            logger_->info("Task: Destroyed task " + name_);
        }
    }

    /**
     * @brief Starts the task by creating a new thread. Registers with the
     * watchdog (if one was given) before the thread starts, so a task that
     * never calls kickWatchdog() at all is still caught by the watchdog's
     * timeout rather than going unmonitored until its first kick.
     */
    virtual void start() {
        if (!running_) {
            running_ = true;
            if (watchdog_) {
                watchdog_->registerParticipant(id_, name_, watchdogTimeout_);
            }
            thread_ = std::thread(&Task::run, this);
            if (logger_) {
                logger_->info("Task: Started task " + name_);
            }
        }
    }

    /**
     * @brief Joins the task thread if it is joinable.
     */
    virtual void join() {
        if (thread_.joinable()) {
            thread_.join();
            if (logger_) {
                logger_->info("Task: Joined task " + name_);
            }
        }
    }

    /**
     * @brief Stops the task, detaches it from the virtual bus, and joins the thread.
     */
    virtual void stop() {
        if (running_) {
            running_ = false;
            bus_.detach(id_);
            if (watchdog_) {
                watchdog_->unregisterParticipant(id_);
            }
            if (thread_.joinable()) {
                thread_.join();
            }
            if (logger_) {
                logger_->info("Task: Stopped task " + name_);
            }
        }
    }

    /**
     * @brief Getter for the task identifier.
     * @return Task identifier.
     */
    int getID() const { return id_; }

    /**
     * @brief Getter for the task name.
     * @return Task name.
     */
    std::string getName() const { return name_; }

protected:
    /**
     * @brief Pure virtual function to run the task logic. Must be implemented by derived classes.
     */
    virtual void run() = 0;

    /**
     * @brief Proves this task is still alive, if a watchdog was given at
     * construction. No-op otherwise -- safe to call unconditionally from
     * run() regardless of whether this particular Task instance has a
     * watchdog, so a subclass's loop doesn't need its own null check.
     */
    void kickWatchdog() {
        if (watchdog_) {
            watchdog_->kick(id_);
        }
    }

    std::string name_;  ///< The name of the task
    VirtualBus& bus_;  ///< Reference to the virtual bus the task interacts with
    std::atomic<bool> running_;  ///< Flag to indicate if the task is running
    std::thread thread_;  ///< Thread for executing the task
    int id_;  ///< Unique identifier for the task
    std::shared_ptr<ILogger> logger_;  ///< Logger instance for logging messages.
                                        ///< Protected (not private) so derived
                                        ///< task classes use this instance
                                        ///< directly instead of declaring their
                                        ///< own same-named member that shadows
                                        ///< it and stays null (see
                                        ///< SendTask/ReceiveTask history).
    std::shared_ptr<Watchdog> watchdog_;  ///< Optional watchdog this task registers with; null means unmonitored.
    std::chrono::milliseconds watchdogTimeout_;  ///< Timeout passed to watchdog_->registerParticipant() in start().
};

#endif // TASK_H
