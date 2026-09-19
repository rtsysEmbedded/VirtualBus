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
 * @brief Delivery priority for a message on the bus.
 *
 * Backed by a small, fixed set of levels (rather than an open-ended
 * numeric priority) so VirtualBus/ThreadPool can dispatch with one FIFO
 * queue per level instead of a comparator-driven priority_queue: cheaper,
 * and FIFO order within a level is preserved deterministically instead of
 * depending on a heap's reordering.
 */
enum class Priority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/// Number of Priority levels. VirtualBus's and ThreadPool's per-priority
/// queue arrays are sized with this, and it also bounds valid
/// static_cast<size_t>(Priority) values used to index them.
constexpr size_t kPriorityLevels = 4;

/**
 * @brief Class representing a virtual bus command.
 */
class VirtualBusCmd {
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
     * @brief Updates the timestamp to the current wall-clock time.
     *
     * Used for messages constructed and inspected outside of a
     * VirtualBus (or in tests). Once a message is actually sent via
     * VirtualBus::sendMessage(), that call overwrites the timestamp
     * again using the bus's own (possibly injected, e.g. VirtualClock)
     * IClock -- see updateTimestamp(uint64_t) below -- so the timestamp
     * a receiver observes always reflects "when this entered the bus"
     * rather than "when the command object happened to be constructed".
     */
    void updateTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto tse = now.time_since_epoch();
        auto millisecondsTime = std::chrono::duration_cast<std::chrono::milliseconds>(tse);
        updateTimestamp(static_cast<uint64_t>(millisecondsTime.count()));
    }

    /**
     * @brief Sets the timestamp to an explicit value (milliseconds since
     * an implementation-defined epoch -- see IClock::nowMs()).
     *
     * @param[in] nowMs The timestamp to record.
     */
    void updateTimestamp(uint64_t nowMs) {
        timestamp_ = nowMs;
        if (logger_) {
            logger_->info("VirtualBusCmd: Timestamp updated to " + std::to_string(timestamp_));
        }
    }

    /**
     * @brief Setter for the message's delivery priority.
     * @param[in] priority The priority to set.
     */
    void setPriority(Priority priority) { priority_ = priority; }

    /**
     * @brief Getter for the message's delivery priority.
     * @return Message priority (Normal if never explicitly set).
     */
    Priority getPriority() const { return priority_; }

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
    Priority priority_ = Priority::Normal;  ///< Delivery priority of the command
    std::shared_ptr<ILogger> logger_;  ///< Logger instance for logging messages.
                                        ///< Protected (not private) so derived
                                        ///< command classes use this instance
                                        ///< directly instead of declaring their
                                        ///< own same-named member that shadows
                                        ///< it and stays null (see
                                        ///< InverterCommand history).

private:
    std::shared_ptr<JsonCmdParser> parser_;  ///< Parser for JSON command parsing
};

#endif // VIRTUAL_BUS_COMMAND_H
