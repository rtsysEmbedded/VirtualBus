/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef SEND_TASK_H
#define SEND_TASK_H

#include "Task.h"
#include "InverterCommand.h"
#include "ILogger.h"
#include "ErrorHandler.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

/**
 * @brief Class representing a task for sending inverter commands to the virtual bus.
 */
class SendTask : public Task {
public:
    /**
     * @brief Constructor for SendTask.
     *
     * @param[in] bus The virtual bus reference.
     * @param[in] id Task identifier.
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     * @param[in] watchdog Optional watchdog (see Task's constructor); left
     * null, this task isn't monitored, same as before this parameter existed.
     * @param[in] watchdogTimeout How long this task may run without an
     * iteration completing before the watchdog reports it. Only meaningful
     * when `watchdog` is non-null.
     */
    SendTask(const std::string& name, VirtualBus& bus, std::shared_ptr<ILogger> logger = nullptr,
             std::shared_ptr<Watchdog> watchdog = nullptr,
             std::chrono::milliseconds watchdogTimeout = std::chrono::milliseconds(5000))
        : Task(name, bus, logger, std::move(watchdog), watchdogTimeout) {}

protected:
    /**
     * @brief Main logic of the SendTask that runs in a loop, sending commands to the virtual bus.
     */
    void run() override {
        while (running_) {
            kickWatchdog();
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto command = std::make_shared<InverterCommand>();
            command->setCurrent(10.0);
            command->setVoltage(54.6);
            command->updateTimestamp();
            command->setMode(InverterCommand::Mode::Charging);

            if (bus_.sendMessage(id_, command) == ReturnType::OK) {
                if (logger_) {
                    logger_->info("SendTask: Sent InverterCommand (Charging): Voltage = " + std::to_string(command->getVoltage()) + ", Current = " + std::to_string(command->getCurrent()));
                }
            } else if (logger_) {
                logger_->warn("SendTask: Failed to send InverterCommand.");
            }
        }
        if (logger_) {
            logger_->info("SendTask: Stopped sending commands.");
        }
    }
};

#endif // SEND_TASK_H
