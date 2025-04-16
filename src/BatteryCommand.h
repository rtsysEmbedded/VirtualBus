/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef BATTERY_COMMAND_H
#define BATTERY_COMMAND_H

#include "VirtualBusCmd.h"
#include "nlohmann/json.hpp"
#include "BatteryCommandParser.h"
#include "ILogger.h"
#include <limits>
#include <memory>
#include <string>

/**
 * @brief Class representing a battery state command.
 */
class BatteryStateCmd : public VirtualBusCmd {
private:
    uint8_t numberOfCubes = 0;
    uint8_t numberOfReadyCubes = 0;
    uint16_t voltageMinimum = std::numeric_limits<uint16_t>::max();
    uint16_t voltageMaximum = std::numeric_limits<uint16_t>::min();
    int16_t voltageMean = 0;
    uint16_t socMaximum = std::numeric_limits<uint16_t>::min();
    uint16_t socMinimum = std::numeric_limits<uint16_t>::max();
    uint32_t socMean = 0;
    int32_t currentSum = 0;
    int32_t currentMean = 0;
    int32_t currentMinimum = std::numeric_limits<int32_t>::max();
    int32_t currentMaximum = std::numeric_limits<int32_t>::min();
    int16_t temperatureMinimum = std::numeric_limits<int16_t>::max();
    int16_t temperatureMaximum = std::numeric_limits<int16_t>::min();
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Constructor initializing command type as Server.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    BatteryStateCmd(std::shared_ptr<ILogger> logger = nullptr) : VirtualBusCmd(), logger_(logger) {
        type = CommandType::Server;
    }

    /**
     * @brief Initializes the parser for the battery state command.
     */
    void initializeParser() {
        setParser(std::make_shared<BatteryCommandParser>());
        if (logger_) logger_->info("BatteryStateCmd: Parser initialized.");
    }

    /**
     * @brief Getter for the number of battery cubes.
     * @return Number of battery cubes.
     */
    uint8_t getNumberOfCubes() const { return numberOfCubes; }

    /**
     * @brief Getter for the number of ready battery cubes.
     * @return Number of ready battery cubes.
     */
    uint8_t getNumberOfReadyCubes() const { return (numberOfReadyCubes == 0) ? 0 : numberOfReadyCubes; }

    /**
     * @brief Getter for the minimum voltage.
     * @return Minimum voltage.
     */
    uint16_t getVoltageMinimum() const { return (voltageMinimum < 48000) ? 48000 : voltageMinimum; }

    /**
     * @brief Getter for the maximum voltage.
     * @return Maximum voltage.
     */
    uint16_t getVoltageMaximum() const { return (voltageMaximum > 57000) ? 57000 : voltageMaximum; }

    /**
     * @brief Getter for the mean voltage.
     * @return Mean voltage.
     */
    uint16_t getVoltageMean() const { return voltageMaximum + voltageMinimum / 2; }

    /**
     * @brief Getter for the minimum state of charge (SOC).
     * @return Minimum SOC.
     */
    uint16_t getSOCMinimum() const {
        if (socMinimum > 10000) return 10000;
        if (socMinimum < 50) return 300;
        return socMinimum;
    }

    /**
     * @brief Getter for the maximum state of charge (SOC).
     * @return Maximum SOC.
     */
    uint16_t getSOCMaximum() const {
        if (socMaximum > 10000) return 10000;
        if (socMaximum < 50) return 300;
        return socMaximum;
    }

    /**
     * @brief Getter for the mean state of charge (SOC).
     * @return Mean SOC.
     */
    uint16_t getSOCMean() const {
        if (socMean > 10000) return 10000;
        if (socMean < 50) return 300;
        return socMean;
    }

    /**
     * @brief Getter for the minimum current.
     * @return Minimum current.
     */
    int32_t getCurrentMinimum() const { return currentMinimum; }

    /**
     * @brief Getter for the maximum current.
     * @return Maximum current.
     */
    int32_t getCurrentMaximum() const { return currentMaximum; }

    /**
     * @brief Getter for the mean current.
     * @return Mean current.
     */
    int32_t getCurrentMean() const { return currentMean; }

    /**
     * @brief Getter for the current sum.
     * @return Sum of current.
     */
    int32_t getCurrentSum() const { return currentSum; }

    /**
     * @brief Getter for the maximum temperature.
     * @return Maximum temperature.
     */
    uint16_t getTemperatureMaximum() const { return temperatureMaximum; }

    /**
     * @brief Getter for the minimum temperature.
     * @return Minimum temperature.
     */
    uint16_t getTemperatureMinimum() const { return temperatureMinimum; }

    /**
     * @brief Resets all statistics to initial values.
     */
    void resetStatistics() {
        numberOfCubes = 0;
        numberOfReadyCubes = 0;
        voltageMinimum = std::numeric_limits<uint16_t>::max();
        voltageMaximum = std::numeric_limits<uint16_t>::min();
        socMaximum = std::numeric_limits<uint16_t>::min();
        socMinimum = std::numeric_limits<uint16_t>::max();
        socMean = 0;
        currentSum = 0;
        currentMean = 0;
        currentMinimum = std::numeric_limits<int32_t>::max();
        currentMaximum = std::numeric_limits<int32_t>::min();
        temperatureMinimum = std::numeric_limits<int16_t>::max();
        temperatureMaximum = std::numeric_limits<int16_t>::min();
        if (logger_) logger_->info("BatteryStateCmd: Statistics reset to initial values.");
    }

    /**
     * @brief Converts battery state to JSON.
     * @return JSON representation of the battery state.
     */
    nlohmann::json toJson() const {
        nlohmann::json jsonRepresentation;

        jsonRepresentation["Cube_Num"] = numberOfCubes;
        jsonRepresentation["Cube_OP"] = numberOfReadyCubes;
        jsonRepresentation["Current"] = {
            {"AVG", currentMean},
            {"MAX", currentMaximum},
            {"MIN", currentMinimum}
        };
        jsonRepresentation["DATE"] = "22222";
        jsonRepresentation["Device_id"] = "82475923";
        jsonRepresentation["SOC"] = {
            {"AVG", getSOCMean()},
            {"MAX", getSOCMaximum()},
            {"MIN", getSOCMinimum()}
        };
        jsonRepresentation["Voltage"] = {
            {"AVG", voltageMean},
            {"MAX", getVoltageMaximum()},
            {"MIN", getVoltageMinimum()}
        };

        return jsonRepresentation;
    }

    /**
     * @brief Prints the battery state in JSON format.
     */
    void print() const override {
        print_base();
        if (logger_) logger_->info("BatteryStateCmd: Printing battery state as JSON.");
        std::cout << toJson().dump(4) << std::endl; // Pretty print with 4 spaces indentation
    }
};

#endif // BATTERY_COMMAND_H
