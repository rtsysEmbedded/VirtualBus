// ErrorHandler.cpp
#include "ErrorHandler.h"

std::mutex ErrorHandler::mutex_;  // Define the static mutex variable here.

void ErrorHandler::handleError(const std::string& module, const std::string& message, ErrorSeverity severity, std::shared_ptr<ILogger> logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logger) {
        switch (severity) {
            case ErrorSeverity::INFO:
                logger->info("[INFO] [" + module + "] " + message);
                break;
            case ErrorSeverity::WARNING:
                logger->warn("[WARNING] [" + module + "] " + message);
                break;
            case ErrorSeverity::ERROR:
                logger->error("[ERROR] [" + module + "] " + message);
                break;
            case ErrorSeverity::CRITICAL:
                logger->critical("[CRITICAL] [" + module + "] " + message);
                std::terminate(); // Optionally terminate the program for safety-critical systems
                break;
        }
    } else {
        switch (severity) {
            case ErrorSeverity::INFO:
                std::cout << "[INFO] [" << module << "] " << message << std::endl;
                break;
            case ErrorSeverity::WARNING:
                std::cerr << "[WARNING] [" << module << "] " << message << std::endl;
                break;
            case ErrorSeverity::ERROR:
                std::cerr << "[ERROR] [" << module << "] " << message << std::endl;
                break;
            case ErrorSeverity::CRITICAL:
                std::cerr << "[CRITICAL] [" << module << "] " << message << std::endl;
                // For critical errors, additional actions might be taken
                std::terminate(); // Optionally terminate the program for safety-critical systems
                break;
        }
    }
}
