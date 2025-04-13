#ifndef I_LOGGER_H
#define I_LOGGER_H

#include <string>

/**
 * @brief Interface representing a generic logger.
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /**
     * @brief Log an informational message.
     *
     * @param[in] message The message to log.
     */
    virtual void info(const std::string& message) = 0;

    /**
     * @brief Log a warning message.
     *
     * @param[in] message The message to log.
     */
    virtual void warn(const std::string& message) = 0;

    /**
     * @brief Log an error message.
     *
     * @param[in] message The message to log.
     */
    virtual void error(const std::string& message) = 0;

    /**
     * @brief Log a critical error message.
     *
     * @param[in] message The message to log.
     */
    virtual void critical(const std::string& message) = 0;
};

#endif // I_LOGGER_H
