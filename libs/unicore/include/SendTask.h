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
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Constructor for SendTask.
     *
     * @param[in] bus The virtual bus reference.
     * @param[in] id Task identifier.
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    SendTask(const std::string& name, VirtualBus& bus, std::shared_ptr<ILogger> logger = nullptr)
        : Task(name,bus,logger) {}

protected:
    /**
     * @brief Main logic of the SendTask that runs in a loop, sending commands to the virtual bus.
     */
    void run() override {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto command = std::make_shared<InverterCommand>();
            command->setCurrent(10.0);
            command->setVoltage(54.6);
            command->updateTimestamp();
            command->setMode(InverterCommand::Mode::Charging);

            bus_.sendMessage(id_, command);
            if (logger_) {
                logger_->info("SendTask: Sent InverterCommand (Charging): Voltage = " + std::to_string(command->getVoltage()) + ", Current = " + std::to_string(command->getCurrent()));
            }
        }
        if (logger_) {
            logger_->info("SendTask: Stopped sending commands.");
        }
    }
};

#endif // SEND_TASK_H
