#ifndef DIAGNOSTIC_TASK_H
#define DIAGNOSTIC_TASK_H

#include "Task.h"

#ifdef SPDLOG_LIB
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#endif
/**
 * @brief Class representing a diagnostic task that monitors system health.
 */
class DiagnosticTask : public Task {
public:
    /**
     * @brief Constructor for DiagnosticTask.
     *
     * @param[in] name The name of the diagnostic task.
     * @param[in] bus Reference to the virtual bus.
     */
    DiagnosticTask(const std::string& name, VirtualBus& bus)
        : Task(name, bus) {
#ifdef SPDLOG_LIB
        logger_ = spdlog::basic_logger_mt("diagnostic_logger", "logs/diagnostic.log");
        logger_->set_level(spdlog::level::info);
        logger_->info("DiagnosticTask initialized.");
#endif
    }

protected:
    /**
     * @brief Main logic of the DiagnosticTask that periodically checks the system.
     */
    void run() override {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            performDiagnostics();
        }
    }

    /**
     * @brief Perform diagnostic checks on the system and log results.
     */
    void performDiagnostics() {
        // Example diagnostic checks
        if (bus_.isRunning()) {
#ifdef SPDLOG_LIB
            logger_->info("VirtualBus is running smoothly.");
#endif
        } else {
#ifdef SPDLOG_LIB
            logger_->error("VirtualBus is not running!");
#endif
        }
#ifdef SPDLOG_LIB
        // You can add further diagnostics checks, such as CPU usage, memory, etc.
        logger_->info("Diagnostic check completed at {}", spdlog::to_string(std::chrono::system_clock::now()));
#endif
    }

private:
#ifdef SPDLOG_LIB
    std::shared_ptr<spdlog::logger> logger_; ///< Logger instance for the diagnostic task.
#endif
};

#endif // DIAGNOSTIC_TASK_H
