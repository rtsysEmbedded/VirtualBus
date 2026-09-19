# VirtualBus Usage Guide

This guide explains how to use VirtualBus and its components in your applications.

## Table of Contents

1. [Basic Concepts](#basic-concepts)
2. [Quick Start](#quick-start)
3. [Core Components](#core-components)
4. [Working with Tasks](#working-with-tasks)
5. [Message Passing](#message-passing)
6. [Configuration](#configuration)
7. [Logging](#logging)
8. [Error Handling](#error-handling)
9. [Advanced Usage](#advanced-usage)
10. [Common Patterns](#common-patterns)

---

## Basic Concepts

### Virtual Bus Architecture

The VirtualBus is a **message broker** that enables inter-task communication:

- **Tasks**: Independent execution units that can send and receive messages
- **Messages**: Structured command objects passed between tasks
- **Callbacks**: Event handlers triggered when messages arrive
- **Queue**: Thread-safe message queue for each task

### Message Flow

```
SendTask → [Task 1 ID: 1] ──┐
                             ├─→ VirtualBus ──→ Message Queue ──→ ReceiveTask
Battery Task → [Task 2 ID: 2] ┘      (Broker)      (Task 3)
```

---

## Quick Start

### Minimal Example

```cpp
#include "VirtualBus.h"
#include "SpdLogWrapper.h"

int main() {
    // Create logger
    auto logger = std::make_shared<SpdLogWrapper>();
    
    // Create virtual bus
    VirtualBus bus(logger);
    
    // Attach a task
    bus.attach(1, "MyTask");
    
    // Send a message
    auto msg = std::make_shared<VirtualBusCmd>();
    bus.sendMessage(1, msg);
    
    // Receive a message
    std::shared_ptr<VirtualBusCmd> receivedMsg;
    if (bus.receiveMessage(1, receivedMsg)) {
        logger->info("Message received!");
    }
    
    return 0;
}
```

---

## Core Components

### 1. VirtualBus Class

The central message broker for all task communication.

#### Construction

```cpp
// With logger
auto logger = std::make_shared<SpdLogWrapper>();
VirtualBus bus(logger);

// Without logger (default)
VirtualBus bus;
```

#### Task Management

```cpp
// Attach a task to the bus
ReturnType result = bus.attach(taskId, taskName);
if (result == ReturnType::OK) {
    std::cout << "Task attached successfully" << std::endl;
}

// Detach a task from the bus
bus.detach(taskId);
```

#### Message Sending

```cpp
// Create a command message
auto command = std::make_shared<VirtualBusCmd>();

// Send message from sender to bus
bus.sendMessage(senderId, command);
```

#### Message Receiving

```cpp
// Receive message for a task
std::shared_ptr<VirtualBusCmd> message;
bool received = bus.receiveMessage(taskId, message);

if (received) {
    // Process message
    message->print();
}
```

#### Callback Registration

```cpp
// Register callback for event-driven message handling
bus.registerCallback(taskId, [](std::shared_ptr<VirtualBusCmd> cmd) {
    std::cout << "Message received in callback" << std::endl;
    cmd->print();
});
```

#### Shutdown

```cpp
// Gracefully shutdown the bus
bus.shutdown();
```

### 2. Task Class

Base class for runnable components in the system.

#### Creating a Custom Task

```cpp
#include "Task.h"

class MyTask : public Task {
private:
    std::shared_ptr<ILogger> logger_;
    
public:
    MyTask(const std::string& name, VirtualBus& bus, 
           std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), logger_(logger) {}
    
protected:
    void run() override {
        while (running_) {
            // Your task logic here
            if (logger_) {
                logger_->info("MyTask is running");
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
```

#### Task Lifecycle

```cpp
// Create task
MyTask task("MyTask", bus, logger);

// Start task (runs in separate thread)
task.start();

// Get task information
int id = task.getID();
std::string name = task.getName();

// Stop task (signals thread to stop)
task.stop();

// Wait for task completion
task.join();
```

### 3. VirtualBusCmd Class

Base class for all command/message objects.

#### Creating Custom Commands

```cpp
// Include the command base class
#include "VirtualBusCmd.h"

// Example: InverterCommand
class MyCommand : public VirtualBusCmd {
private:
    double value_;
    
public:
    void setValue(double v) { value_ = v; }
    double getValue() const { return value_; }
    
    void print() const override {
        std::cout << "MyCommand: value=" << value_ << std::endl;
    }
};
```

#### Using Commands

```cpp
// Create command instance
auto cmd = std::make_shared<MyCommand>();
cmd->setValue(42.0);

// Send via bus
bus.sendMessage(senderId, cmd);

// Receive and use
std::shared_ptr<VirtualBusCmd> received;
if (bus.receiveMessage(receiverId, received)) {
    if (auto myCmd = std::dynamic_pointer_cast<MyCommand>(received)) {
        std::cout << "Value: " << myCmd->getValue() << std::endl;
    }
}
```

---

## Working with Tasks

### Sender Task

A task that periodically sends commands to the bus.

```cpp
class ProducerTask : public Task {
protected:
    void run() override {
        int counter = 0;
        while (running_) {
            auto cmd = std::make_shared<VirtualBusCmd>();
            bus_.sendMessage(id_, cmd);
            
            counter++;
            if (logger_) {
                logger_->info("Sent message #" + std::to_string(counter));
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
```

### Receiver Task

A task that processes incoming commands.

```cpp
class ConsumerTask : public Task {
public:
    void start() override {
        // Register callback before starting
        bus_.registerCallback(id_, [this](std::shared_ptr<VirtualBusCmd> cmd) {
            this->onMessageReceived(cmd);
        });
        Task::start();
    }
    
protected:
    void run() override {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
private:
    void onMessageReceived(std::shared_ptr<VirtualBusCmd> cmd) {
        if (logger_) {
            logger_->info("Message received");
        }
        cmd->print();
    }
};
```

### Multiple Tasks

```cpp
int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    // Create multiple tasks
    ProducerTask producer("Producer", bus, logger);
    ConsumerTask consumer("Consumer", bus, logger);
    
    // Attach all tasks
    bus.attach(producer.getID(), producer.getName());
    bus.attach(consumer.getID(), consumer.getName());
    
    // Start all tasks
    producer.start();
    consumer.start();
    
    // Let them run
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Stop all tasks
    producer.stop();
    consumer.stop();
    
    // Cleanup
    bus.shutdown();
    producer.join();
    consumer.join();
    
    return 0;
}
```

---

## Message Passing

### Synchronous Message Passing

Receive messages from the queue:

```cpp
// Non-blocking receive
std::shared_ptr<VirtualBusCmd> msg;
if (bus.receiveMessage(taskId, msg)) {
    std::cout << "Got message" << std::endl;
    msg->print();
} else {
    std::cout << "No message available" << std::endl;
}
```

### Asynchronous Message Passing

Use callbacks for event-driven processing:

```cpp
// Register callback
bus.registerCallback(taskId, [logger](std::shared_ptr<VirtualBusCmd> msg) {
    logger->info("Async message received");
    msg->print();
});

// Message handler is called automatically when message arrives
```

### Message Types

#### Built-in Commands

```cpp
// Basic command
auto basicCmd = std::make_shared<VirtualBusCmd>();
bus.sendMessage(senderId, basicCmd);

// Inverter command
auto inverterCmd = std::make_shared<InverterCommand>();
inverterCmd->setVoltage(48.0);
inverterCmd->setCurrent(15.0);
inverterCmd->setMode(InverterCommand::Mode::Charging);
bus.sendMessage(senderId, inverterCmd);

// Battery command
auto batteryCmd = std::make_shared<BatteryCommand>();
batteryCmd->setSOC(85.0);  // State of Charge
batteryCmd->setTemperature(25.0);
bus.sendMessage(senderId, batteryCmd);
```

#### Creating Custom Command Types

```cpp
#include "VirtualBusCmd.h"

class TemperatureCommand : public VirtualBusCmd {
private:
    double temperature_;
    
public:
    void setTemperature(double temp) {
        temperature_ = temp;
        updateTimestamp();
    }
    
    double getTemperature() const {
        return temperature_;
    }
    
    void print() const override {
        std::cout << "Temperature: " << temperature_ << "°C" << std::endl;
    }
};
```

---

## Configuration

### Loading Configuration

```cpp
#include "Configuration.h"
#include "JsonStorage.h"

// Get singleton instance
Configuration& config = Configuration::getInstance();

// Set logger
config.setLogger(logger);

// Set storage backend
auto jsonStorage = std::make_unique<JsonStorage>(logger);
config.setStorage(std::move(jsonStorage));

// Load configuration from file
if (config.load("config.json")) {
    std::cout << "Configuration loaded successfully" << std::endl;
} else {
    std::cerr << "Failed to load configuration" << std::endl;
}
```

### Accessing Configuration

```cpp
// Get configuration value
std::string logLevel = config.getConfig("log_level");
std::string maxThreads = config.getConfig("max_threads");
std::string mqttBroker = config.getConfig("mqtt_broker");

// Use configuration values
if (logLevel == "debug") {
    // Enable debug logging
}
```

### Modifying Configuration

```cpp
// Set configuration value
config.setConfig("log_level", "debug");
config.setConfig("max_threads", "8");

// Save to file
if (config.save("updated_config.json")) {
    std::cout << "Configuration saved" << std::endl;
}
```

### Configuration File Format (JSON)

```json
{
    "log_level": "info",
    "max_threads": "4",
    "mqtt_broker": "mqtt://localhost:1883",
    "mqtt_client_id": "virtualbus_client",
    "task_timeout": "5000",
    "enable_watchdog": true,
    "watchdog_timeout": "30000"
}
```

---

## Logging

### Logger Types

#### spdlog Wrapper (Production)

```cpp
#include "SpdLogWrapper.h"

auto logger = std::make_shared<SpdLogWrapper>();

// Log messages at different levels
logger->debug("Debug message");
logger->info("Information message");
logger->warn("Warning message");
logger->error("Error message");
logger->critical("Critical error");
```

#### stdout Logger (Development)

```cpp
#include "StdCoutLogger.h"

auto logger = std::make_shared<StdCoutLogger>();

// Same interface as spdlog
logger->info("This is printed to stdout");
```

### Conditional Logging

```cpp
#ifdef USE_SPDLOG
auto logger = std::make_shared<SpdLogWrapper>();
#else
auto logger = std::make_shared<StdCoutLogger>();
#endif
```

### Using Logger in Your Code

```cpp
class MyTask : public Task {
private:
    std::shared_ptr<ILogger> logger_;
    
public:
    MyTask(const std::string& name, VirtualBus& bus,
           std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), logger_(logger) {}
    
protected:
    void run() override {
        if (logger_) {
            logger_->info("Task started");
        }
        
        // Task logic...
        
        if (logger_) {
            logger_->info("Task completed");
        }
    }
};
```

---

## Error Handling

### Error Handler Class

```cpp
#include "ErrorHandler.h"

// Handle error with different severity levels
ErrorHandler::handleError(
    "ComponentName",
    "Error description",
    ErrorHandler::ErrorSeverity::ERROR,
    logger
);
```

### Error Severity Levels

```cpp
// INFO: Non-critical information
ErrorHandler::handleError(..., ErrorHandler::ErrorSeverity::INFO, logger);

// WARNING: Potential issues that don't stop execution
ErrorHandler::handleError(..., ErrorHandler::ErrorSeverity::WARNING, logger);

// ERROR: Critical errors requiring immediate attention
ErrorHandler::handleError(..., ErrorHandler::ErrorSeverity::ERROR, logger);

// FATAL: Unrecoverable errors requiring shutdown
ErrorHandler::handleError(..., ErrorHandler::ErrorSeverity::FATAL, logger);
```

### Return Types

```cpp
#include "ReturnType.h"

// Check return values
ReturnType result = bus.attach(taskId, taskName);
if (result == ReturnType::OK) {
    // Success
} else if (result == ReturnType::ERROR) {
    // Error occurred
} else if (result == ReturnType::TIMEOUT) {
    // Operation timed out
}
```

### Try-Catch Error Handling

```cpp
try {
    // Risky operation
    if (bus.attach(taskId, taskName) != ReturnType::OK) {
        throw std::runtime_error("Failed to attach task");
    }
    
    // More operations...
    
} catch (const std::exception& e) {
    if (logger_) {
        logger_->error(std::string("Exception: ") + e.what());
    }
    ErrorHandler::handleError("MyTask", e.what(),
                            ErrorHandler::ErrorSeverity::ERROR, logger);
}
```

---

## Advanced Usage

### Thread Pool

```cpp
#include "ThreadPool.h"

// Create thread pool with specified number of threads
ThreadPool pool(4);  // 4 worker threads

// Enqueue work at a priority (0 = lowest, ThreadPool::kNumPriorityLevels - 1
// = highest; ThreadPool::kDefaultPriority is a reasonable default)
pool.enqueue(ThreadPool::kDefaultPriority, []() {
    std::cout << "Work done by thread pool" << std::endl;
});
```

### Diagnostic Task

```cpp
#include "DiagnosticTask.h"

// Create diagnostic task for system monitoring
DiagnosticTask diagnostic("Diagnostic", bus, logger);
bus.attach(diagnostic.getID(), diagnostic.getName());
diagnostic.start();

// Diagnostic task monitors system health...
```

### Custom Command Parser

```cpp
#include "JsonCmdParser.h"

// Parse JSON to command
std::string jsonStr = R"({"type": "inverter", "voltage": 48.0})";
auto cmd = JsonCmdParser::parse(jsonStr);

if (cmd) {
    bus.sendMessage(senderId, cmd);
}
```

---

## Common Patterns

### Producer-Consumer Pattern

```cpp
class Producer : public Task {
protected:
    void run() override {
        for (int i = 0; i < 10; i++) {
            auto msg = std::make_shared<VirtualBusCmd>();
            bus_.sendMessage(id_, msg);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

class Consumer : public Task {
public:
    void start() override {
        bus_.registerCallback(id_, [this](auto msg) {
            this->processMessage(msg);
        });
        Task::start();
    }
    
private:
    void processMessage(std::shared_ptr<VirtualBusCmd> msg) {
        // Process message
    }
};
```

### Periodic Task Pattern

```cpp
class PeriodicTask : public Task {
private:
    std::chrono::seconds interval_;
    
public:
    PeriodicTask(const std::string& name, VirtualBus& bus,
                std::chrono::seconds interval,
                std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), interval_(interval) {}
    
protected:
    void run() override {
        while (running_) {
            // Do periodic work
            std::cout << "Periodic work done" << std::endl;
            
            // Wait for next period
            std::this_thread::sleep_for(interval_);
        }
    }
};
```

### Request-Response Pattern

```cpp
class RequestTask : public Task {
public:
    void start() override {
        bus_.registerCallback(id_, [this](auto response) {
            this->handleResponse(response);
        });
        Task::start();
    }
    
protected:
    void run() override {
        auto request = std::make_shared<VirtualBusCmd>();
        bus_.sendMessage(id_, request);
        
        // Wait for response
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
private:
    void handleResponse(std::shared_ptr<VirtualBusCmd> response) {
        // Handle response from receiver
    }
};
```

### State Machine Pattern

```cpp
class StatefulTask : public Task {
private:
    enum class State { IDLE, RUNNING, STOPPED };
    State state_ = State::IDLE;
    
protected:
    void run() override {
        while (running_) {
            switch (state_) {
                case State::IDLE:
                    onIdle();
                    break;
                case State::RUNNING:
                    onRunning();
                    break;
                case State::STOPPED:
                    onStopped();
                    break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
private:
    void onIdle() { /* Handle idle state */ }
    void onRunning() { /* Handle running state */ }
    void onStopped() { /* Handle stopped state */ }
};
```

---

## Performance Tips

### 1. Use Callbacks for High-Frequency Messages

```cpp
// Good: Callback-based (event-driven)
bus.registerCallback(taskId, [](auto msg) {
    // Handle immediately when message arrives
});

// Less efficient: Polling
while (running_) {
    std::shared_ptr<VirtualBusCmd> msg;
    if (bus.receiveMessage(taskId, msg)) {
        // Process message
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

### 2. Batch Operations

```cpp
// Process multiple messages efficiently
class BatchConsumer : public Task {
protected:
    void run() override {
        std::vector<std::shared_ptr<VirtualBusCmd>> batch;
        
        while (running_) {
            // Collect messages
            std::shared_ptr<VirtualBusCmd> msg;
            while (bus_.receiveMessage(id_, msg)) {
                batch.push_back(msg);
            }
            
            // Process batch
            processBatch(batch);
            batch.clear();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
private:
    void processBatch(const std::vector<std::shared_ptr<VirtualBusCmd>>& msgs) {
        // Process all messages in batch
    }
};
```

### 3. Optimize Thread Count

```cpp
// Match thread count to CPU cores
int numCores = std::thread::hardware_concurrency();
ThreadPool pool(numCores);
```

---

## Debugging

### Enable Logging

```cpp
// Set log level to debug
config.setConfig("log_level", "debug");

// Use SpdLogWrapper for detailed logging
auto logger = std::make_shared<SpdLogWrapper>();
```

### Message Tracing

```cpp
// Add tracing in message handlers
bus.registerCallback(taskId, [logger](auto msg) {
    logger->debug("Message received: " + msg->getType());
    msg->print();
});
```

### Thread Debugging

```cpp
// Log thread information
logger->info("Current thread ID: " + 
            std::to_string(std::this_thread::get_id()));
```

---

## Next Steps

- Review [ARCHITECTURE.md](./ARCHITECTURE.md) for detailed system design
- Check [EXAMPLES.md](./EXAMPLES.md) for more code examples
- Read [CONFIGURATION.md](./CONFIGURATION.md) for all config options
- See [TROUBLESHOOTING.md](./TROUBLESHOOTING.md) for common issues

---

**Happy coding with VirtualBus!**
