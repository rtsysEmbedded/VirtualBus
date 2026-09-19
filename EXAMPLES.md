# VirtualBus Examples

Practical code examples demonstrating various VirtualBus features and patterns.

## Table of Contents

1. [Hello World](#hello-world)
2. [Basic Message Passing](#basic-message-passing)
3. [Custom Tasks](#custom-tasks)
4. [Custom Commands](#custom-commands)
5. [Configuration Usage](#configuration-usage)
6. [Error Handling](#error-handling)
7. [Advanced Patterns](#advanced-patterns)
8. [Real-World Scenarios](#real-world-scenarios)

---

## Hello World

### Minimal Application

The simplest VirtualBus application:

```cpp
#include <iostream>
#include "VirtualBus.h"
#include "SpdLogWrapper.h"

int main() {
    // Create logger
    auto logger = std::make_shared<SpdLogWrapper>();
    
    // Create virtual bus
    VirtualBus bus(logger);
    
    // Create a message
    auto msg = std::make_shared<VirtualBusCmd>();
    
    // Send message
    bus.sendMessage(1, msg);
    
    logger->info("Hello from VirtualBus!");
    
    return 0;
}
```

### Expected Output

```
[2025-09-19 10:30:45.123] [info] Hello from VirtualBus!
```

---

## Basic Message Passing

### Synchronous Polling

Send and receive messages using polling:

```cpp
#include "VirtualBus.h"
#include "SpdLogWrapper.h"
#include <thread>
#include <chrono>

int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    // Attach sender task
    bus.attach(1, "Sender");
    bus.attach(2, "Receiver");
    
    // Send messages
    for (int i = 0; i < 5; i++) {
        auto msg = std::make_shared<VirtualBusCmd>();
        bus.sendMessage(1, msg);
        logger->info("Sent message " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Receive messages
    std::shared_ptr<VirtualBusCmd> received;
    while (bus.receiveMessage(2, received)) {
        logger->info("Received message");
        received->print();
    }
    
    return 0;
}
```

### Asynchronous Callbacks

Handle messages with callbacks:

```cpp
#include "VirtualBus.h"
#include "SpdLogWrapper.h"
#include <thread>
#include <chrono>
#include <atomic>

std::atomic<int> messageCount(0);

void messageHandler(std::shared_ptr<VirtualBusCmd> msg) {
    messageCount++;
    std::cout << "Callback: Received message #" << messageCount << std::endl;
    msg->print();
}

int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    // Attach tasks
    bus.attach(1, "Sender");
    bus.attach(2, "Receiver");
    
    // Register callback for receiver
    bus.registerCallback(2, messageHandler);
    
    // Send messages
    for (int i = 0; i < 5; i++) {
        auto msg = std::make_shared<VirtualBusCmd>();
        bus.sendMessage(1, msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Give callbacks time to process
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "Total messages processed: " << messageCount << std::endl;
    
    return 0;
}
```

---

## Custom Tasks

### Simple Producer Task

A task that periodically produces messages:

```cpp
#include "Task.h"
#include "VirtualBus.h"
#include "ILogger.h"
#include <thread>
#include <chrono>

class ProducerTask : public Task {
private:
    std::shared_ptr<ILogger> logger_;
    int messageCount_;
    
public:
    ProducerTask(const std::string& name, VirtualBus& bus,
                std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), logger_(logger), messageCount_(0) {}
    
protected:
    void run() override {
        while (running_) {
            messageCount_++;
            auto msg = std::make_shared<VirtualBusCmd>();
            bus_.sendMessage(id_, msg);
            
            if (logger_) {
                logger_->info("ProducerTask: Sent message #" + 
                            std::to_string(messageCount_));
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

// Usage
int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    ProducerTask producer("Producer", bus, logger);
    bus.attach(producer.getID(), producer.getName());
    
    producer.start();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    producer.stop();
    producer.join();
    
    return 0;
}
```

### Consumer Task with Callback

A task that processes incoming messages:

```cpp
#include "Task.h"
#include "VirtualBus.h"
#include "ILogger.h"
#include <atomic>

class ConsumerTask : public Task {
private:
    std::shared_ptr<ILogger> logger_;
    std::atomic<int> messageCount_(0);
    
public:
    ConsumerTask(const std::string& name, VirtualBus& bus,
                std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), logger_(logger) {}
    
    void start() override {
        // Register callback
        bus_.registerCallback(id_, [this](std::shared_ptr<VirtualBusCmd> msg) {
            this->onMessageReceived(msg);
        });
        Task::start();
    }
    
    int getMessageCount() const { return messageCount_; }
    
protected:
    void run() override {
        while (running_) {
            if (logger_) {
                logger_->info("ConsumerTask: Processing...");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
private:
    void onMessageReceived(std::shared_ptr<VirtualBusCmd> msg) {
        messageCount_++;
        if (logger_) {
            logger_->info("ConsumerTask: Processed message #" + 
                        std::to_string(messageCount_));
        }
        msg->print();
    }
};

// Usage
int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    ConsumerTask consumer("Consumer", bus, logger);
    bus.attach(consumer.getID(), consumer.getName());
    
    consumer.start();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    consumer.stop();
    consumer.join();
    
    std::cout << "Total messages: " << consumer.getMessageCount() << std::endl;
    
    return 0;
}
```

---

## Custom Commands

### Temperature Sensor Command

Create a custom command for temperature data:

```cpp
#include "VirtualBusCmd.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

class TemperatureSensorCommand : public VirtualBusCmd {
private:
    double temperature_;
    double humidity_;
    std::string location_;
    
public:
    TemperatureSensorCommand() 
        : temperature_(0.0), humidity_(0.0), location_("Unknown") {}
    
    void setTemperature(double temp) {
        temperature_ = temp;
        updateTimestamp();
    }
    
    void setHumidity(double hum) {
        humidity_ = hum;
        updateTimestamp();
    }
    
    void setLocation(const std::string& loc) {
        location_ = loc;
        updateTimestamp();
    }
    
    double getTemperature() const { return temperature_; }
    double getHumidity() const { return humidity_; }
    std::string getLocation() const { return location_; }
    
    void print() const override {
        std::cout << "TemperatureSensorCommand: "
                  << "Location=" << location_
                  << ", Temp=" << temperature_ << "°C"
                  << ", Humidity=" << humidity_ << "%"
                  << std::endl;
    }
};

// Usage
int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    bus.attach(1, "Sensor");
    bus.attach(2, "DataCollector");
    
    // Create and send temperature command
    auto tempCmd = std::make_shared<TemperatureSensorCommand>();
    tempCmd->setTemperature(23.5);
    tempCmd->setHumidity(65.0);
    tempCmd->setLocation("Room A");
    
    bus.sendMessage(1, tempCmd);
    tempCmd->print();
    
    return 0;
}
```

### Battery Status Command

Custom command for battery system:

```cpp
#include "VirtualBusCmd.h"
#include <iostream>

class BatteryStatusCommand : public VirtualBusCmd {
public:
    enum class Status { Charging, Discharging, Idle };
    
private:
    double stateOfCharge_;  // 0-100%
    double voltage_;        // Volts
    double current_;        // Amps
    Status status_;
    double temperature_;    // °C
    
public:
    BatteryStatusCommand()
        : stateOfCharge_(50.0), voltage_(48.0), current_(0.0),
          status_(Status::Idle), temperature_(25.0) {}
    
    void setStateOfCharge(double soc) { stateOfCharge_ = soc; }
    void setVoltage(double v) { voltage_ = v; }
    void setCurrent(double i) { current_ = i; }
    void setStatus(Status s) { status_ = s; }
    void setTemperature(double t) { temperature_ = t; }
    
    double getStateOfCharge() const { return stateOfCharge_; }
    double getVoltage() const { return voltage_; }
    double getCurrent() const { return current_; }
    Status getStatus() const { return status_; }
    double getTemperature() const { return temperature_; }
    
    void print() const override {
        std::string statusStr;
        switch (status_) {
            case Status::Charging: statusStr = "Charging"; break;
            case Status::Discharging: statusStr = "Discharging"; break;
            case Status::Idle: statusStr = "Idle"; break;
        }
        
        std::cout << "BatteryStatus: "
                  << "SOC=" << stateOfCharge_ << "%"
                  << ", V=" << voltage_ << "V"
                  << ", I=" << current_ << "A"
                  << ", Status=" << statusStr
                  << ", Temp=" << temperature_ << "°C"
                  << std::endl;
    }
};

// Usage
int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    bus.attach(1, "BatteryMonitor");
    bus.attach(2, "BatteryController");
    
    auto batteryStatus = std::make_shared<BatteryStatusCommand>();
    batteryStatus->setStateOfCharge(85.0);
    batteryStatus->setVoltage(48.5);
    batteryStatus->setCurrent(10.0);
    batteryStatus->setStatus(BatteryStatusCommand::Status::Charging);
    batteryStatus->setTemperature(28.5);
    
    bus.sendMessage(1, batteryStatus);
    batteryStatus->print();
    
    return 0;
}
```

---

## Configuration Usage

### Loading and Using Configuration

```cpp
#include "Configuration.h"
#include "JsonStorage.h"
#include "VirtualBus.h"

int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    
    // Setup configuration
    Configuration& config = Configuration::getInstance();
    config.setLogger(logger);
    config.setStorage(std::make_unique<JsonStorage>(logger));
    
    // Load configuration file
    if (!config.load("config.json")) {
        logger->error("Failed to load configuration");
        return -1;
    }
    
    // Read configuration values
    std::string logLevel = config.getConfig("log_level");
    std::string maxThreadsStr = config.getConfig("max_threads");
    std::string mqttBroker = config.getConfig("mqtt_broker");
    
    // Convert and validate
    int maxThreads = std::stoi(maxThreadsStr);
    bool enableWatchdog = config.getConfig("enable_watchdog") == "true";
    
    // Log configuration
    logger->info("Configuration loaded:");
    logger->info("  Log Level: " + logLevel);
    logger->info("  Max Threads: " + maxThreadsStr);
    logger->info("  MQTT Broker: " + mqttBroker);
    logger->info("  Watchdog Enabled: " + 
                std::string(enableWatchdog ? "true" : "false"));
    
    // Update configuration
    config.setConfig("log_level", "debug");
    config.setConfig("current_session_id", "session_001");
    
    // Save updated configuration
    if (config.save("updated_config.json")) {
        logger->info("Configuration saved successfully");
    }
    
    return 0;
}
```

### Configuration File Example

**config.json**:
```json
{
    "log_level": "info",
    "max_threads": "4",
    "mqtt_broker": "mqtt://localhost:1883",
    "mqtt_client_id": "virtualbus_example",
    "task_timeout": "5000",
    "enable_watchdog": true,
    "watchdog_timeout": "30000"
}
```

---

## Error Handling

### Basic Error Handling

```cpp
#include "VirtualBus.h"
#include "ErrorHandler.h"
#include "SpdLogWrapper.h"

int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    // Try to attach task
    ReturnType result = bus.attach(1, "MyTask");
    if (result != ReturnType::OK) {
        ErrorHandler::handleError("Main", "Failed to attach task",
                                ErrorHandler::ErrorSeverity::ERROR, logger);
        return -1;
    }
    
    logger->info("Task attached successfully");
    
    return 0;
}
```

### Try-Catch Error Handling

```cpp
#include "VirtualBus.h"
#include <stdexcept>

int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    
    try {
        VirtualBus bus(logger);
        
        // Simulate an error condition
        if (bus.attach(1, "Task1") != ReturnType::OK) {
            throw std::runtime_error("Failed to attach task");
        }
        
        logger->info("All operations successful");
        
    } catch (const std::exception& e) {
        logger->error(std::string("Exception: ") + e.what());
        ErrorHandler::handleError("Main", e.what(),
                                ErrorHandler::ErrorSeverity::FATAL, logger);
        return -1;
    }
    
    return 0;
}
```

### Comprehensive Error Handling

```cpp
class SafeTask : public Task {
private:
    std::shared_ptr<ILogger> logger_;
    
public:
    SafeTask(const std::string& name, VirtualBus& bus,
            std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), logger_(logger) {}
    
protected:
    void run() override {
        try {
            while (running_) {
                try {
                    doWork();
                } catch (const std::exception& e) {
                    if (logger_) {
                        logger_->error(std::string("Work failed: ") + e.what());
                    }
                    // Continue running, skip this iteration
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } catch (const std::exception& e) {
            if (logger_) {
                logger_->critical(std::string("Fatal error: ") + e.what());
            }
            ErrorHandler::handleError("SafeTask", e.what(),
                                    ErrorHandler::ErrorSeverity::FATAL, logger_);
        }
    }
    
private:
    void doWork() {
        // Task work
        if (logger_) {
            logger_->info("Task work completed");
        }
    }
};
```

---

## Advanced Patterns

### Producer-Consumer Pattern

```cpp
#include "Task.h"
#include "VirtualBus.h"
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>

class AdvancedProducer : public Task {
private:
    std::shared_ptr<ILogger> logger_;
    
public:
    AdvancedProducer(const std::string& name, VirtualBus& bus,
                    std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), logger_(logger) {}
    
protected:
    void run() override {
        for (int i = 1; i <= 10; i++) {
            auto msg = std::make_shared<VirtualBusCmd>();
            bus_.sendMessage(id_, msg);
            
            if (logger_) {
                logger_->info("Producer: Produced item " + std::to_string(i));
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
};

class AdvancedConsumer : public Task {
private:
    std::shared_ptr<ILogger> logger_;
    int itemsConsumed_;
    
public:
    AdvancedConsumer(const std::string& name, VirtualBus& bus,
                    std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), logger_(logger), itemsConsumed_(0) {}
    
    void start() override {
        bus_.registerCallback(id_, [this](std::shared_ptr<VirtualBusCmd> msg) {
            this->onItemReceived(msg);
        });
        Task::start();
    }
    
    int getItemsConsumed() const { return itemsConsumed_; }
    
protected:
    void run() override {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
private:
    void onItemReceived(std::shared_ptr<VirtualBusCmd> msg) {
        itemsConsumed_++;
        if (logger_) {
            logger_->info("Consumer: Consumed item " + 
                        std::to_string(itemsConsumed_));
        }
    }
};

// Usage
int main() {
    auto logger = std::make_shared<SpdLogWrapper>();
    VirtualBus bus(logger);
    
    AdvancedProducer producer("Producer", bus, logger);
    AdvancedConsumer consumer("Consumer", bus, logger);
    
    bus.attach(producer.getID(), producer.getName());
    bus.attach(consumer.getID(), consumer.getName());
    
    producer.start();
    consumer.start();
    
    producer.join();
    
    consumer.stop();
    consumer.join();
    
    std::cout << "Items consumed: " << consumer.getItemsConsumed() << std::endl;
    
    return 0;
}
```

### Periodic Task with Configuration

```cpp
class ConfigurablePeriodicTask : public Task {
private:
    std::shared_ptr<ILogger> logger_;
    std::chrono::milliseconds interval_;
    
public:
    ConfigurablePeriodicTask(const std::string& name, VirtualBus& bus,
                           std::shared_ptr<ILogger> logger = nullptr)
        : Task(name, bus, logger), logger_(logger), interval_(1000) {
        
        // Load interval from configuration
        Configuration& config = Configuration::getInstance();
        std::string intervalStr = config.getConfig("task_interval", "1000");
        interval_ = std::chrono::milliseconds(std::stoi(intervalStr));
    }
    
protected:
    void run() override {
        while (running_) {
            performPeriodicWork();
            std::this_thread::sleep_for(interval_);
        }
    }
    
private:
    void performPeriodicWork() {
        if (logger_) {
            logger_->info("Performing periodic work...");
        }
        
        auto msg = std::make_shared<VirtualBusCmd>();
        bus_.sendMessage(id_, msg);
    }
};
```

---

## Real-World Scenarios

### Scenario 1: Battery Management System

```cpp
// Battery management system monitoring and control
class BatteryManagementSystem {
public:
    BatteryManagementSystem() {
        logger_ = std::make_shared<SpdLogWrapper>();
        bus_ = std::make_unique<VirtualBus>(logger_);
        
        // Create tasks
        chargeController_ = std::make_shared<ChargeControllerTask>(
            "ChargeController", *bus_, logger_);
        monitor_ = std::make_shared<BatteryMonitorTask>(
            "Monitor", *bus_, logger_);
        protectionSystem_ = std::make_shared<ProtectionTask>(
            "Protection", *bus_, logger_);
        
        // Attach tasks
        bus_->attach(chargeController_->getID(), chargeController_->getName());
        bus_->attach(monitor_->getID(), monitor_->getName());
        bus_->attach(protectionSystem_->getID(), protectionSystem_->getName());
    }
    
    void start() {
        logger_->info("BMS: Starting");
        chargeController_->start();
        monitor_->start();
        protectionSystem_->start();
    }
    
    void stop() {
        logger_->info("BMS: Stopping");
        chargeController_->stop();
        monitor_->stop();
        protectionSystem_->stop();
        
        chargeController_->join();
        monitor_->join();
        protectionSystem_->join();
        
        bus_->shutdown();
    }
    
private:
    std::shared_ptr<ILogger> logger_;
    std::unique_ptr<VirtualBus> bus_;
    std::shared_ptr<ChargeControllerTask> chargeController_;
    std::shared_ptr<BatteryMonitorTask> monitor_;
    std::shared_ptr<ProtectionTask> protectionSystem_;
};

// Usage
int main() {
    BatteryManagementSystem bms;
    
    bms.start();
    std::this_thread::sleep_for(std::chrono::seconds(30));
    bms.stop();
    
    return 0;
}
```

### Scenario 2: Multi-Device Communication

```cpp
// Multiple device coordination system
class DeviceCoordinator {
public:
    DeviceCoordinator() {
        logger_ = std::make_shared<SpdLogWrapper>();
        bus_ = std::make_unique<VirtualBus>(logger_);
    }
    
    void addDevice(int deviceId, std::shared_ptr<Task> device) {
        devices_[deviceId] = device;
        bus_->attach(device->getID(), device->getName());
    }
    
    void startAllDevices() {
        for (auto& [id, device] : devices_) {
            device->start();
        }
    }
    
    void stopAllDevices() {
        for (auto& [id, device] : devices_) {
            device->stop();
        }
        for (auto& [id, device] : devices_) {
            device->join();
        }
        bus_->shutdown();
    }
    
private:
    std::shared_ptr<ILogger> logger_;
    std::unique_ptr<VirtualBus> bus_;
    std::unordered_map<int, std::shared_ptr<Task>> devices_;
};
```

---

## Running the Examples

### Compile and Run

```bash
# Build
cd build
cmake ..
make

# Run example
./CPPProject
```

### With Custom Configuration

```bash
# Create custom config
cat > example_config.json << EOF
{
    "log_level": "debug",
    "max_threads": "4"
}
EOF

# Run with config
./CPPProject
```

---

**More examples in the `src/` directory!**
