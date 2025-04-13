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
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Enumeration representing inverter mode.
     */
    enum class Mode { Charging, Discharging } mode;

    double current = 0.0;  ///< Current value in amperes
    double voltage = 0.0;  ///< Voltage value in volts
    CommandType type;

    /**
     * @brief Default constructor initializing command type as Inverter and default mode as Charging.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    InverterCommand(std::shared_ptr<ILogger> logger = nullptr) : VirtualBusCmd(), logger_(logger) {
        type = CommandType::Inverter;
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
