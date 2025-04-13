/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#include <iostream>
#include <chrono>
#include <thread>

#include "VirtualBus.h"
#include "SendTask.h"
#include "ReciveTask.h"

#include "Configuration.h"
#include "JsonStorage.h"
#include "ErrorHandler.h"

#include "OD.h"
// Conditionally include SpdLogWrapper or StdCoutLogger
#ifdef USE_SPDLOG
#include "SpdLogWrapper.h"
#else
#include "StdCoutLogger.h"
#endif

/**
 * @brief Main function to initialize and start send and receive tasks, and manage their lifecycle.
 *
 * @return Exit code.
 */
int main() {
    std::cout<<"###############################################"<<std::endl;
    std::cout<<"Project Version:"<<PROJECT_VERSION<<std::endl;
    std::cout<<"###############################################"<<std::endl;
    // Set the logger
#ifdef USE_SPDLOG
    auto logger = std::make_shared<SpdLogWrapper>();
    std::cout<<"spdlog set for logging"<<std::endl;
#else
    auto logger = std::make_shared<StdCoutLogger>();
#endif

    VirtualBus bus(logger);

    // Get the singleton instance of Configuration
    Configuration& config = Configuration::getInstance();

    config.setLogger(logger);

    // Set the storage backend to JSON storage
    auto jsonStorage = std::make_unique<JsonStorage>(logger);
    config.setStorage(std::move(jsonStorage));

    // Load configuration from JSON file
    if (!config.load("config.json")) {
        ErrorHandler::handleError("Main", "Failed to load configuration file.", ErrorHandler::ErrorSeverity::ERROR, logger);
        return -1;
    }
    // Retrieve configuration values
    std::string logLevel = config.getConfig("log_level");
    std::string maxThreads = config.getConfig("max_threads");

    logger->info("Log Level: " + logLevel);
    logger->info("Max Threads: " + maxThreads);

    // Initialize sender and receiver tasks
    SendTask sender("Sender", bus, logger);
    ReceiveTask receiver("Receiver", bus, logger);

    // Attach tasks to the virtual bus
    if (bus.attach(sender.getID(), sender.getName()) != ReturnType::OK) {
        ErrorHandler::handleError("Main", "Failed to attach sender task.", ErrorHandler::ErrorSeverity::ERROR, logger);
        return -1;
    }
    if (bus.attach(receiver.getID(), receiver.getName()) != ReturnType::OK) {
        ErrorHandler::handleError("Main", "Failed to attach receiver task.", ErrorHandler::ErrorSeverity::ERROR, logger);
        return -1;
    }

    // Start the tasks
    sender.start();
    receiver.start();

    // Let the tasks run for a defined duration
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop the tasks
    sender.stop();
    receiver.stop();

    // Shutdown the bus to stop any waiting threads
    bus.shutdown();

    // Join the tasks to ensure clean shutdown
    sender.join();
    receiver.join();

    // Update configuration value
    config.setConfig("log_level", "debug");
    if (!config.save("updated_config.json")) {
        ErrorHandler::handleError("Main", "Failed to save updated configuration file.", ErrorHandler::ErrorSeverity::ERROR, logger);
    }

    return 0;
}
