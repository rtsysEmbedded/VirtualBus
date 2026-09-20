/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef BATTERY_COMMAND_H
#define BATTERY_COMMAND_H

#include "VirtualBusCmd.h"
#include "nlohmann/json.hpp"
#include "JsonCmdParser.h"
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
    uint16_t voltageMean = 0;
    uint16_t socMaximum = std::numeric_limits<uint16_t>::min();
    uint16_t socMinimum = std::numeric_limits<uint16_t>::max();
    uint32_t socMean = 0;
    int32_t currentSum = 0;
    int32_t currentMean = 0;
    int32_t currentMinimum = std::numeric_limits<int32_t>::max();
    int32_t currentMaximum = std::numeric_limits<int32_t>::min();
    int16_t temperatureMinimum = std::numeric_limits<int16_t>::max();
    int16_t temperatureMaximum = std::numeric_limits<int16_t>::min();

public:
    /**
     * @brief Constructor initializing command type as Battery.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    BatteryStateCmd(std::shared_ptr<ILogger> logger = nullptr) : VirtualBusCmd(logger) {
        type_ = CommandType::Battery;
    }

    /**
     * @brief Initializes the parser for the battery state command.
     *
     * Defined in BatteryCommand.cpp rather than inline here, because
     * building it needs BatteryCommandParser.h, which itself includes
     * BatteryCommand.h to reference BatteryStateCmd. If BatteryCommand.h
     * included BatteryCommandParser.h back (as it originally did), the
     * include guards would make whichever header started the cycle win:
     * for a translation unit that includes BatteryCommand.h first, that
     * header's own include of BatteryCommandParser.h would run before the
     * BatteryStateCmd class it needs to reference was fully defined,
     * which doesn't compile. Splitting the definition out (the same
     * pattern InverterCommand.cpp already uses for
     * InverterCommand::initializeParser()) avoids the cycle entirely.
     */
    void initializeParser();

    /**
     * @brief Setter for the number of battery cubes.
     * @param[in] value Number of battery cubes.
     */
    void setNumberOfCubes(uint8_t value) {
        numberOfCubes = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set Cube_Num to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the number of ready battery cubes.
     * @param[in] value Number of ready battery cubes.
     */
    void setNumOfReadyCubes(uint8_t value) {
        numberOfReadyCubes = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set Cube_OP to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the minimum voltage.
     * @param[in] value Minimum voltage.
     */
    void setMinVoltage(uint16_t value) {
        voltageMinimum = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set Voltage MIN to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the maximum voltage.
     * @param[in] value Maximum voltage.
     */
    void setMaxVoltage(uint16_t value) {
        voltageMaximum = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set Voltage MAX to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the mean state of charge (SOC).
     * @param[in] value Mean SOC.
     */
    void setMeanSOC(uint32_t value) {
        socMean = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set SOC AVG to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the minimum state of charge (SOC).
     * @param[in] value Minimum SOC.
     */
    void setMinSOC(uint16_t value) {
        socMinimum = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set SOC MIN to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the maximum state of charge (SOC).
     * @param[in] value Maximum SOC.
     */
    void setMaxSOC(uint16_t value) {
        socMaximum = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set SOC MAX to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the mean voltage.
     * @param[in] value Mean voltage.
     */
    void setMeanVoltage(uint16_t value) {
        voltageMean = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set Voltage AVG to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the mean current.
     * @param[in] value Mean current.
     */
    void setMeanCurrent(int32_t value) {
        currentMean = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set Current AVG to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the minimum current.
     * @param[in] value Minimum current.
     */
    void setMinCurrent(int32_t value) {
        currentMinimum = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set Current MIN to " + std::to_string(value));
        }
    }

    /**
     * @brief Setter for the maximum current.
     * @param[in] value Maximum current.
     */
    void setMaxCurrent(int32_t value) {
        currentMaximum = value;
        if (logger_) {
            logger_->info("BatteryStateCmd: Set Current MAX to " + std::to_string(value));
        }
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
    uint16_t getVoltageMean() const { return (voltageMaximum + voltageMinimum) / 2; }

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
     * @brief Serializes battery state for RemoteBridge forwarding.
     *
     * Reuses toJson() -- the same shape BatteryCommandParser already
     * round-trips from an external JSON command source -- rather than a
     * separate format.
     */
    nlohmann::json serializePayload() const override {
        return toJson();
    }

    /**
     * @brief Populates battery state fields from a peer's
     * serializePayload() (i.e. a toJson()-shaped object).
     *
     * Reads the same fields BatteryCommandParser::parseParameters()
     * reads from an external JSON command, applied here directly via the
     * existing setters -- deliberately not routed through
     * BatteryCommandParser itself, since that would need
     * BatteryCommandParser.h, which is exactly the header cycle
     * initializeParser() was split out-of-line to avoid (see its doc
     * comment above).
     * @return True unless `payload` isn't a JSON object at all.
     */
    bool deserializePayload(const nlohmann::json& payload) override {
        if (!payload.is_object()) {
            return false;
        }
        if (payload.contains("Cube_Num")) {
            setNumberOfCubes(payload["Cube_Num"].get<uint8_t>());
        }
        if (payload.contains("Cube_OP")) {
            setNumOfReadyCubes(payload["Cube_OP"].get<uint8_t>());
        }
        if (payload.contains("Voltage") && payload["Voltage"].is_object()) {
            const auto& v = payload["Voltage"];
            if (v.contains("MIN")) setMinVoltage(v["MIN"].get<uint16_t>());
            if (v.contains("MAX")) setMaxVoltage(v["MAX"].get<uint16_t>());
            if (v.contains("AVG")) setMeanVoltage(v["AVG"].get<uint16_t>());
        }
        if (payload.contains("SOC") && payload["SOC"].is_object()) {
            const auto& soc = payload["SOC"];
            if (soc.contains("AVG")) setMeanSOC(soc["AVG"].get<uint32_t>());
            if (soc.contains("MIN")) setMinSOC(soc["MIN"].get<uint16_t>());
            if (soc.contains("MAX")) setMaxSOC(soc["MAX"].get<uint16_t>());
        }
        if (payload.contains("Current") && payload["Current"].is_object()) {
            const auto& c = payload["Current"];
            if (c.contains("AVG")) setMeanCurrent(c["AVG"].get<int32_t>());
            if (c.contains("MIN")) setMinCurrent(c["MIN"].get<int32_t>());
            if (c.contains("MAX")) setMaxCurrent(c["MAX"].get<int32_t>());
        }
        return true;
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
        printBase();
        if (logger_) logger_->info("BatteryStateCmd: Printing battery state as JSON.");
        std::cout << toJson().dump(4) << std::endl; // Pretty print with 4 spaces indentation
    }
};

#endif // BATTERY_COMMAND_H
