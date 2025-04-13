// ErrorHandler.h
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <iostream>
#include <string>
#include <mutex>
#include <memory>
#include "ILogger.h"

class ErrorHandler {
public:
    enum class ErrorSeverity {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    static void handleError(const std::string& module, const std::string& message, ErrorSeverity severity, std::shared_ptr<ILogger> logger = nullptr);

private:
    static std::mutex mutex_;  // Declare as static, but without initialization.
};

#endif // ERROR_HANDLER_H
