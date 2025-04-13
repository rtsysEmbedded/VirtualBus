#ifndef STD_COUT_LOGGER_H
#define STD_COUT_LOGGER_H

#include "ILogger.h"
#include <iostream>
#include <mutex>
#include <string>

/**
 * @brief Class representing a basic console logger that implements the `ILogger` interface using `std::cout`.
 */
class StdCoutLogger : public ILogger {
public:
    /**
     * @brief Log an informational message.
     *
     * @param[in] message The message to log.
     */
    void info(const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[INFO]: " << message << std::endl;
    }

    /**
     * @brief Log a warning message.
     *
     * @param[in] message The message to log.
     */
    void warn(const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[WARNING]: " << message << std::endl;
    }

    /**
     * @brief Log an error message.
     *
     * @param[in] message The message to log.
     */
    void error(const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << "[ERROR]: " << message << std::endl;
    }

    /**
     * @brief Log a critical error message.
     *
     * @param[in] message The message to log.
     */
    void critical(const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << "[CRITICAL]: " << message << std::endl;
    }

private:
    std::mutex mutex_; ///< Mutex to ensure thread-safe logging
};

#endif // STD_COUT_LOGGER_H
