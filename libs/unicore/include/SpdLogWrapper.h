#ifndef SPDLOG_WRAPPER_H
#define SPDLOG_WRAPPER_H

#include "ILogger.h"
#include <spdlog/spdlog.h>
#include spdlog/sinks/basic_file_sink.h>
#include <memory>

/**
 * @brief Class representing an `spdlog` wrapper that implements the `ILogger` interface.
 */
class SpdLogWrapper : public ILogger {
public:
    /**
     * @brief Constructor for SpdLogWrapper.
     */
    SpdLogWrapper() {
        logger_ = spdlog::basic_logger_mt("logger", "logs/application.log");
        logger_->set_level(spdlog::level::info);  // Default log level
    }

    /**
     * @brief Log an informational message.
     *
     * @param[in] message The message to log.
     */
    void info(const std::string& message) override {
        logger_->info(message);
    }

    /**
     * @brief Log a warning message.
     *
     * @param[in] message The message to log.
     */
    void warn(const std::string& message) override {
        logger_->warn(message);
    }

    /**
     * @brief Log an error message.
     *
     * @param[in] message The message to log.
     */
    void error(const std::string& message) override {
        logger_->error(message);
    }

    /**
     * @brief Log a critical error message.
     *
     * @param[in] message The message to log.
     */
    void critical(const std::string& message) override {
        logger_->critical(message);
    }

private:
    std::shared_ptr<spdlog::logger> logger_; ///< `spdlog` logger instance
};

#endif // SPDLOG_WRAPPER_H
