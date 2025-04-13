/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef VIRTUAL_BUS_COMMAND_H
#define VIRTUAL_BUS_COMMAND_H

#include <memory>
#include <string>
#include <chrono>
#include <iostream>
#include "ILogger.h"

class JsonCmdParser;
/**
 * @brief Enumeration representing command types.
 */
enum class CommandType {
    Inverter = 0,
    Battery,
    Gateway,
    Json
};

/**
 * @brief Class representing a virtual bus command.
 */
class VirtualBusCmd {
private:
    std::shared_ptr<ILogger> logger_; ///< Logger instance for logging messages

public:
    /**
     * @brief Default constructor for VirtualBusCmd.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    VirtualBusCmd(std::shared_ptr<ILogger> logger = nullptr)
        : commandString_(""), timestamp_(0), type_(CommandType::Json), logger_(logger) {
        updateTimestamp();
        if (logger_) {
            logger_->info("VirtualBusCmd: Command created with default type Json.");
        }
    }

    /**
     * @brief Updates the timestamp to the current time.
     */
    void updateTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto tse = now.time_since_epoch();
        auto millisecondsTime = std::chrono::duration_cast<std::chrono::milliseconds>(tse);
        timestamp_ = uint64_t(millisecondsTime.count());
        if (logger_) {
            logger_->info("VirtualBusCmd: Timestamp updated to " + std::to_string(timestamp_));
        }
    }

    /**
     * @brief Setter for the command parser.
     *
     * @param[in] parser Shared pointer to a JSON command parser.
     */
    void setParser(std::shared_ptr<JsonCmdParser> parser) {
        parser_ = parser;
        if (logger_) {
            logger_->info("VirtualBusCmd: Parser set.");
        }
    }

    /**
     * @brief Parses the given parameters using the injected parser.
     *
     * @param[in] parameters JSON string representing command parameters.
     * @return True if parsing is successful, otherwise false.
     */
    bool parse(const std::string& parameters) ;

    /**
     * @brief Pure virtual function to print the command details.
     */
    virtual void print() const = 0;

    /**
     * @brief Virtual destructor for VirtualBusCmd.
     */
    virtual ~VirtualBusCmd() {
        if (logger_) {
            logger_->info("VirtualBusCmd: Command destroyed.");
        }
    }

    /**
     * @brief Getter for the timestamp of the command.
     * @return Command timestamp.
     */
    uint64_t getTimestamp() const { return timestamp_; }

    /**
     * @brief Getter for the command type.
     * @return Command type.
     */
    CommandType getType() const { return type_; }

protected:
    /**
     * @brief Prints the base command details.
     */
    void printBase() const {
        std::cout << "VirtualBusCmd: " << commandString_ << std::endl;
        std::cout << "Timestamp: " << timestamp_ << std::endl;
        if (logger_) {
            logger_->info("VirtualBusCmd: Printed base command details.");
        }
    }

    std::string commandString_;  ///< Command string representing the command details
    uint64_t timestamp_ = 0;  ///< Timestamp of the command
    CommandType type_;  ///< Type of the command

private:
    std::shared_ptr<JsonCmdParser> parser_;  ///< Parser for JSON command parsing
};

#endif // VIRTUAL_BUS_COMMAND_H
