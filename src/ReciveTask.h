/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef RECEIVE_TASK_H
#define RECEIVE_TASK_H

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
 * @brief Class representing a task for receiving commands from the virtual bus.
 */
class ReceiveTask : public Task {
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Constructor for ReceiveTask.
     *
     * @param[in] bus The virtual bus reference.
     * @param[in] id Task identifier.
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    ReceiveTask(const std::string& name, VirtualBus& bus, std::shared_ptr<ILogger> logger = nullptr)
        : Task(name,bus,logger) {}

    /**
     * @brief Starts the task and registers a callback to handle incoming messages.
     */
    void start() override {
        // Register the callback before starting the thread
        bus_.registerCallback(id_, [this](std::shared_ptr<VirtualBusCmd> cmd) {
            this->onMessageReceived(cmd);
        });
        if (logger_) {
            logger_->info("ReceiveTask: Callback registered and task started.");
        }
        Task::start();
    }

protected:
    /**
     * @brief Main logic of the ReceiveTask that runs in a loop.
     */
    void run() override {
        while (running_) {
            // Perform other tasks if needed
            if (logger_) {
                logger_->info("ReceiveTask: Running...");
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (logger_) {
            logger_->info("ReceiveTask: Stopped running.");
        }
    }

    /**
     * @brief Callback function to handle received messages from the virtual bus.
     *
     * @param[in] cmd Shared pointer to the received command.
     */
    void onMessageReceived(std::shared_ptr<VirtualBusCmd> cmd) {
        if (!running_) return; // Check if the task is still running

        // Process the received command
        if (logger_) {
            logger_->info("ReceiveTask: Message received.");
        }
        cmd->print();
        if (cmd->getType() == CommandType::Inverter) {
            if (auto inverterCmd = std::dynamic_pointer_cast<InverterCommand>(cmd)) {
                inverterCmd->print();
                double voltage = inverterCmd->getVoltage();
                double current = inverterCmd->getCurrent();
                auto mode = inverterCmd->getMode();
                std::string modeString = (mode == InverterCommand::Mode::Charging) ? "Charging" : "Discharging";
                if (logger_) {
                    logger_->info("ReceiveTask: Received InverterCommand (" + modeString + "): Voltage = " + std::to_string(voltage) + ", Current = " + std::to_string(current));
                } else {
                    std::cout << "Received InverterCommand (" << modeString << "): Voltage = " << voltage << ", Current = " << current << std::endl;
                }
            } else {
                if (logger_) {
                    logger_->error("ReceiveTask: Received unknown inverter command type.");
                } else {
                    std::cout << "Received unknown inverter command type." << std::endl;
                }
            }
        }
    }
};

#endif // RECEIVE_TASK_H
