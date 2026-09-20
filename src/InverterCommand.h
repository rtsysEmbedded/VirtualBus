/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef INVERTER_COMMAND_H
#define INVERTER_COMMAND_H

#include "VirtualBusCmd.h"
#include "JsonCmdParser.h"
#include "ILogger.h"
#include "ErrorHandler.h"
#include <iostream>
#include <memory>
#include <string>

/**
 * @brief Class representing an inverter command.
 */
class InverterCommand : public VirtualBusCmd {
public:
    /**
     * @brief Enumeration representing inverter mode.
     */
    enum class Mode { Charging, Discharging } mode;

    double current = 0.0;  ///< Current value in amperes
    double voltage = 0.0;  ///< Voltage value in volts

    /**
     * @brief Default constructor initializing command type as Inverter and default mode as Charging.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    InverterCommand(std::shared_ptr<ILogger> logger = nullptr) : VirtualBusCmd(logger) {
        // Sets the inherited protected type_ (read back via getType()), not
        // a same-named member of its own -- a public `CommandType type;`
        // used to shadow it here the same way logger_ was shadowed
        // elsewhere in this codebase (see logger_ history above): the
        // constructor set that shadow field and getType() kept returning
        // VirtualBusCmd's default CommandType::Json forever, silently
        // dead-ending the `cmd->getType() == CommandType::Inverter` branch
        // in ReceiveTask::onMessageReceived() for every real
        // InverterCommand. Caught by ObjectPool_AcquireConstructsA...'s
        // getType() check in tests/test_object_pool.cpp.
        type_ = CommandType::Inverter;
        mode = Mode::Charging; // Default mode
        if (logger_) {
            logger_->info("InverterCommand: Initialized with mode Charging.");
        }
    }

    /**
     * @brief Initializes the parser for the inverter command.
     */
    void initializeParser();

    /**
     * @brief Getter for the voltage value.
     * @return Voltage value in volts.
     */
    double getVoltage() const { return voltage; }

    /**
     * @brief Getter for the current value.
     * @return Current value in amperes.
     */
    double getCurrent() const { return current; }

    /**
     * @brief Setter for the voltage value.
     * @param[in] data Voltage value in volts.
     */
    void setVoltage(double data) {
        voltage = data;
        if (logger_) {
            logger_->info("InverterCommand: Set voltage to " + std::to_string(data) + " V.");
        }
    }

    /**
     * @brief Setter for the current value.
     * @param[in] data Current value in amperes.
     */
    void setCurrent(double data) {
        current = data;
        if (logger_) {
            logger_->info("InverterCommand: Set current to " + std::to_string(data) + " A.");
        }
    }

    /**
     * @brief Setter for the inverter mode.
     * @param[in] m Inverter mode (Charging or Discharging).
     */
    void setMode(Mode m) {
        mode = m;
        std::string modeString = (mode == Mode::Charging) ? "Charging" : "Discharging";
        if (logger_) {
            logger_->info("InverterCommand: Set mode to " + modeString + ".");
        }
    }

    /**
     * @brief Getter for the inverter mode.
     * @return Inverter mode (Charging or Discharging).
     */
    Mode getMode() const { return mode; }

    /**
     * @brief Serializes voltage/current/mode for RemoteBridge forwarding.
     *
     * A separate, direct field mapping from InverterCommandParser's JSON
     * shape (which maps a "command": "StartCharging"/"StartDischarging"
     * string, meant for an external command source) -- this one is for a
     * lossless local round trip between two InverterCommand instances
     * across a RemoteBridge, not for parsing an externally-authored
     * command.
     */
    nlohmann::json serializePayload() const override {
        nlohmann::json payload;
        payload["voltage"] = voltage;
        payload["current"] = current;
        payload["mode"] = (mode == Mode::Charging) ? "Charging" : "Discharging";
        return payload;
    }

    /**
     * @brief Populates voltage/current/mode from a peer's serializePayload().
     * @return False if "mode" is present but neither "Charging" nor
     * "Discharging" -- malformed payload, RemoteBridge should drop it
     * rather than deliver a command left at its default mode.
     */
    bool deserializePayload(const nlohmann::json& payload) override {
        if (payload.contains("voltage") && payload["voltage"].is_number()) {
            voltage = payload["voltage"].get<double>();
        }
        if (payload.contains("current") && payload["current"].is_number()) {
            current = payload["current"].get<double>();
        }
        if (payload.contains("mode") && payload["mode"].is_string()) {
            const std::string modeStr = payload["mode"].get<std::string>();
            if (modeStr == "Charging") {
                mode = Mode::Charging;
            } else if (modeStr == "Discharging") {
                mode = Mode::Discharging;
            } else {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Prints the inverter command details.
     */
    void print() const override {
        std::string modeString = (mode == Mode::Charging) ? "Charging" : "Discharging";
        if (logger_) {
            logger_->info("InverterCommand: Printing inverter command details.");
        }
        std::cout << modeString << " Current: " << current << " A" << std::endl;
        std::cout << modeString << " Voltage: " << voltage << " V" << std::endl;
    }
};

#endif // INVERTER_COMMAND_H
