/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef BATTERY_COMMAND_PARSER_H
#define BATTERY_COMMAND_PARSER_H

#include "JsonCmdParser.h"
#include "BatteryCommand.h"
#include "nlohmann/json.hpp"
#include "ILogger.h"
#include <iostream>
#include <memory>

/**
 * @brief Class representing a parser for battery state commands.
 */
class BatteryCommandParser : public JsonCmdParser {
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Constructor for BatteryCommandParser.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    BatteryCommandParser(std::shared_ptr<ILogger> logger = nullptr) : logger_(logger) {}

    /**
     * @brief Parses the given parameters and sets them in the command.
     *
     * @param[in] command Reference to a command object that parameters will be set to.
     * @param[in] parameters JSON string representing command parameters.
     * @return True if parsing is successful, otherwise false.
     */
    bool parseParameters(VirtualBusCmd& command, const std::string& parameters) override {
        try {
            // Dynamic cast to ensure we are dealing with a BatteryStateCmd
            auto* batteryCommand = dynamic_cast<BatteryStateCmd*>(&command);
            if (!batteryCommand) {
                if (logger_) logger_->error("BatteryCommandParser: VirtualBusCmd is not of type BatteryStateCmd.");
                return false;
            }

            nlohmann::json jsonData = nlohmann::json::parse(parameters);

            if (jsonData.contains("Cube_Num") && jsonData["Cube_Num"].is_number_unsigned()) {
                batteryCommand->setNumberOfCubes(jsonData.at("Cube_Num").get<uint8_t>());
                if (logger_) logger_->info("BatteryCommandParser: Set Cube_Num to " + std::to_string(jsonData.at("Cube_Num").get<uint8_t>()));
            }

            if (jsonData.contains("Cube_OP") && jsonData["Cube_OP"].is_number_unsigned()) {
                batteryCommand->setNumOfReadyCubes(jsonData.at("Cube_OP").get<uint8_t>());
                if (logger_) logger_->info("BatteryCommandParser: Set Cube_OP to " + std::to_string(jsonData.at("Cube_OP").get<uint8_t>()));
            }

            if (jsonData.contains("Voltage")) {
                if (jsonData["Voltage"].contains("MIN")) {
                    batteryCommand->setMinVoltage(jsonData["Voltage"]["MIN"].get<uint16_t>());
                    if (logger_) logger_->info("BatteryCommandParser: Set Voltage MIN to " + std::to_string(jsonData["Voltage"]["MIN"].get<uint16_t>()));
                }
                if (jsonData["Voltage"].contains("MAX")) {
                    batteryCommand->setMaxVoltage(jsonData["Voltage"]["MAX"].get<uint16_t>());
                    if (logger_) logger_->info("BatteryCommandParser: Set Voltage MAX to " + std::to_string(jsonData["Voltage"]["MAX"].get<uint16_t>()));
                }
            }

            if (jsonData.contains("SOC") && jsonData["SOC"].contains("AVG")) {
                batteryCommand->setMeanSOC(jsonData["SOC"]["AVG"].get<uint32_t>());
                if (logger_) logger_->info("BatteryCommandParser: Set SOC AVG to " + std::to_string(jsonData["SOC"]["AVG"].get<uint32_t>()));
            }

            return true;
        } catch (const nlohmann::json::exception& e) {
            if (logger_) logger_->error("BatteryCommandParser: JSON parsing error: " + std::string(e.what()));
            return false;
        }
    }
};

#endif // BATTERY_COMMAND_PARSER_H
