/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef TASK_H
#define TASK_H

#include <string>
#include <thread>
#include <atomic>
#include "VirtualBus.h"
#include "ILogger.h"
#include "ErrorHandler.h"
#include <memory>

/**
 * @brief Class representing a unique task identifier generator.
 */
class TaskID {
public:
    /**
     * @brief Generates a new unique task identifier.
     * @return A unique task identifier.
     */
    static int getID() {
        return lastID_++;
    }

    static std::atomic<int> lastID_;  ///< Static atomic variable to keep track of the last task ID
};

std::atomic<int> TaskID::lastID_{0};

/**
 * @brief Abstract base class representing a generic task.
 */
class Task {
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Constructor to initialize a task with a name and attach it to the virtual bus.
     *
     * @param[in] name The name of the task.
     * @param[in] bus Reference to the virtual bus the task will use.
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    Task(const std::string& name, VirtualBus& bus, std::shared_ptr<ILogger> logger = nullptr)
        : name_(name), bus_(bus), running_(false), logger_(logger) {
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
     * @brief Starts the task by creating a new thread.
     */
    virtual void start() {
        if (!running_) {
            running_ = true;
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

    std::string name_;  ///< The name of the task
    VirtualBus& bus_;  ///< Reference to the virtual bus the task interacts with
    std::atomic<bool> running_;  ///< Flag to indicate if the task is running
    std::thread thread_;  ///< Thread for executing the task
    int id_;  ///< Unique identifier for the task
};

#endif // TASK_H
