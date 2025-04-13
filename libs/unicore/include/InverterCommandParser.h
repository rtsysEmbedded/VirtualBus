/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef INVERTER_COMMAND_PARSER_H
#define INVERTER_COMMAND_PARSER_H

#include "JsonCmdParser.h"
#include "InverterCommand.h"
#include "nlohmann/json.hpp"
#include "ILogger.h"
#include "ErrorHandler.h"
#include <iostream>
#include <memory>
#include <string>

class InverterCommand;
/**
 * @brief Class representing a parser for inverter commands.
 */
class InverterCommandParser : public JsonCmdParser {
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Constructor for InverterCommandParser.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    InverterCommandParser(std::shared_ptr<ILogger> logger = nullptr) : logger_(logger) {}

    /**
     * @brief Parses the given parameters and sets them in the command.
     *
     * @param[in] command Reference to a command object that parameters will be set to.
     * @param[in] parameters JSON string representing command parameters.
     * @return True if parsing is successful, otherwise false.
     */
    bool parseParameters(VirtualBusCmd& command, const std::string& parameters) override {
        // Ensure we are working with an InverterCommand
        auto* inverterCommand = dynamic_cast<InverterCommand*>(&command);
        if (!inverterCommand) {
            if (logger_) {
                logger_->error("InverterCommandParser: VirtualBusCmd is not of type InverterCommand.");
            } else {
                ErrorHandler::handleError("InverterCommandParser", "VirtualBusCmd is not of type InverterCommand.", ErrorHandler::ErrorSeverity::ERROR);
            }
            return false;
        }

        try {
            nlohmann::json jsonData = nlohmann::json::parse(parameters);

            if (jsonData.contains("current") && jsonData["current"].is_number()) {
                inverterCommand->setCurrent(jsonData["current"].get<double>());
                if (logger_) {
                    logger_->info("InverterCommandParser: Set current to " + std::to_string(jsonData["current"].get<double>()) + " A.");
                } else {
                    ErrorHandler::handleError("InverterCommandParser", "Set current to " + std::to_string(jsonData["current"].get<double>()) + " A.", ErrorHandler::ErrorSeverity::INFO);
                }
            }

            if (jsonData.contains("voltage") && jsonData["voltage"].is_number()) {
                inverterCommand->setVoltage(jsonData["voltage"].get<double>());
                if (logger_) {
                    logger_->info("InverterCommandParser: Set voltage to " + std::to_string(jsonData["voltage"].get<double>()) + " V.");
                } else {
                    ErrorHandler::handleError("InverterCommandParser", "Set voltage to " + std::to_string(jsonData["voltage"].get<double>()) + " V.", ErrorHandler::ErrorSeverity::INFO);
                }
            }

            if (jsonData.contains("command") && jsonData["command"].is_string()) {
                std::string commandStr = jsonData["command"].get<std::string>();
                if (commandStr == "StartCharging") {
                    inverterCommand->setMode(InverterCommand::Mode::Charging);
                    if (logger_) {
                        logger_->info("InverterCommandParser: Set mode to Charging.");
                    } else {
                        ErrorHandler::handleError("InverterCommandParser", "Set mode to Charging.", ErrorHandler::ErrorSeverity::INFO);
                    }
                } else if (commandStr == "StartDischarging") {
                    inverterCommand->setMode(InverterCommand::Mode::Discharging);
                    if (logger_) {
                        logger_->info("InverterCommandParser: Set mode to Discharging.");
                    } else {
                        ErrorHandler::handleError("InverterCommandParser", "Set mode to Discharging.", ErrorHandler::ErrorSeverity::INFO);
                    }
                } else {
                    throw std::runtime_error("Invalid command value");
                }
            }
            return true;
        } catch (const nlohmann::json::exception& e) {
            if (logger_) {
                logger_->error("InverterCommandParser: JSON parsing error: " + std::string(e.what()) + " in input: " + parameters);
            } else {
                ErrorHandler::handleError("InverterCommandParser", "JSON parsing error: " + std::string(e.what()) + " in input: " + parameters, ErrorHandler::ErrorSeverity::ERROR);
            }
            return false;
        }
    }
};

#endif // INVERTER_COMMAND_PARSER_H
