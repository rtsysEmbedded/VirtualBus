# VirtualBus System Architecture

Comprehensive documentation of the VirtualBus system design, components, and interactions.

## Table of Contents

1. [System Overview](#system-overview)
2. [Core Components](#core-components)
3. [Architecture Patterns](#architecture-patterns)
4. [Message Flow](#message-flow)
5. [Threading Model](#threading-model)
6. [Module Dependencies](#module-dependencies)
7. [Design Patterns](#design-patterns)
8. [State Diagrams](#state-diagrams)
9. [Performance Considerations](#performance-considerations)
10. [Extension Points](#extension-points)

---

## System Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │  SendTask    │  │ ReceiveTask  │  │  CustomTasks ... │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                  VirtualBus Core Layer                       │
│  ┌──────────────────────────────────────────────────────┐  │
│  │        Virtual Communication Bus (Message Broker)    │  │
│  │  - Task Registry (attach/detach)                     │  │
│  │  - Message Queue Management                         │  │
│  │  - Callback Management                              │  │
│  │  - Thread Synchronization                           │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ Configuration│  │    Logger    │  │   ErrorHandler   │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│                   Foundation Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │  ThreadPool  │  │  Watchdog    │  │  Diagnostic      │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Key Characteristics

- **Modular**: Clear separation of concerns
- **Extensible**: Plugin architecture for custom tasks and commands
- **Thread-Safe**: All components are thread-safe by design
- **Fault-Tolerant**: Comprehensive error handling
- **Observable**: Detailed logging and diagnostics

---

## Core Components

### 1. VirtualBus (Message Broker)

**Purpose**: Central message routing and task coordination

**Responsibilities**:
- Task registration/deregistration
- Message queue management (per task)
- Callback registration and invocation
- Thread synchronization
- Bus lifecycle management

**Key Methods**:
```
attach(taskId, taskName) → ReturnType
detach(taskId) → void
sendMessage(senderId, message) → void
receiveMessage(taskId, message) → bool
registerCallback(taskId, callback) → void
shutdown() → void
```

**Thread Safety**:
- Uses `std::mutex` for critical sections
- Uses `std::condition_variable` for synchronization
- Atomic flags for state management

**Data Structures**:
```cpp
struct TaskInfo {
    std::string name;
    std::queue<std::shared_ptr<VirtualBusCmd>> messageQueue;
    CallbackFunction callback;
};

std::unordered_map<int, TaskInfo> tasks_;  // Task registry
std::mutex busMutex_;                       // Synchronization
std::condition_variable busConditionVariable_;
std::atomic<bool> running_;
```

### 2. Task (Base Class)

**Purpose**: Abstraction for runnable components

**Responsibilities**:
- Thread lifecycle management
- Message sending/receiving via bus
- Task identification

**Key Methods**:
```
start() → void
stop() → void
join() → void
run() → void (virtual, override)
getID() → int
getName() → std::string
```

**Lifecycle**:
```
[Created] → start() → [Running] → stop() → [Stopping] → join() → [Terminated]
```

**Base Implementation**:
```cpp
class Task {
private:
    std::thread thread_;
    std::atomic<bool> running_;
    int id_;
    std::string name_;
    VirtualBus& bus_;
    std::shared_ptr<ILogger> logger_;
    
protected:
    virtual void run() = 0;
    
public:
    virtual void start();
    virtual void stop();
    virtual void join();
};
```

### 3. VirtualBusCmd (Command Base Class)

**Purpose**: Base class for all message/command types

**Responsibilities**:
- Message type identification
- Timestamp management
- Serialization/deserialization (optional)

**Key Methods**:
```
getType() → CommandType
print() → void
updateTimestamp() → void
getTimestamp() → time_t
```

**Custom Implementation Example**:
```cpp
class InverterCommand : public VirtualBusCmd {
private:
    double voltage_;
    double current_;
    Mode mode_;
    
public:
    void setVoltage(double v);
    double getVoltage() const;
    // ... other methods
    
    void print() const override;
};
```

### 4. Configuration (Singleton)

**Purpose**: Application configuration management

**Responsibilities**:
- Load/save configuration
- Retrieve/update settings
- Storage backend abstraction

**Pattern**: Singleton with lazy initialization

**Key Methods**:
```
getInstance() → Configuration&
load(filename) → bool
save(filename) → bool
getConfig(key) → string
setConfig(key, value) → void
setStorage(storage) → void
```

**Storage Abstraction**:
```cpp
class IStorage {
public:
    virtual bool load(const std::string& filename,
                     std::unordered_map<std::string, std::string>& data) = 0;
    virtual bool save(const std::string& filename,
                     const std::unordered_map<std::string, std::string>& data) = 0;
};

class JsonStorage : public IStorage { /* JSON implementation */ };
```

### 5. Logger (Interface-based)

**Purpose**: Unified logging across the system

**Abstraction**: ILogger interface

**Implementations**:
- **SpdLogWrapper**: High-performance structured logging (production)
- **StdCoutLogger**: Simple console logging (development)

**Log Levels**:
```
DEBUG < INFO < WARN < ERROR < CRITICAL
```

**Usage**:
```cpp
logger->debug("Debug message");
logger->info("Info message");
logger->warn("Warning message");
logger->error("Error message");
logger->critical("Critical message");
```

### 6. ErrorHandler (Static Utility)

**Purpose**: Centralized error handling and reporting

**Features**:
- Severity-based error classification
- Structured error reporting
- Integration with logging system

**Severity Levels**:
```
INFO < WARNING < ERROR < FATAL
```

**Usage**:
```cpp
ErrorHandler::handleError(component, description, severity, logger);
```

### 7. ThreadPool (Optional)

**Purpose**: Worker thread management

**Features**:
- Configurable thread count
- Work queue management
- Thread lifecycle handling

**Usage**:
```cpp
ThreadPool pool(4);  // 4 worker threads
pool.submit([]() { /* work */ });
```

---

## Architecture Patterns

### 1. Message-Passing Architecture

**Pattern**: Actor/Message Passing Model

**Characteristics**:
- Decoupled components
- Asynchronous communication
- No shared state between tasks

**Message Flow**:
```
Task A ──send──→ VirtualBus ──queue──→ Task B's Queue
                                       │
                                       └──callback──→ Handler
                                       └──poll──────→ receiveMessage()
```

### 2. Pub-Sub Pattern (Callbacks)

**Event-Driven Communication**:
```
Task A                Task B                Task C
  │                     │                     │
  └────registerCallback─→ VirtualBus ←───────┘
                          │
                        (message arrives)
                          │
                        (callback fired)
                          │
                       Task B Handler
```

### 3. Singleton Pattern (Configuration)

**Ensures Single Instance**:
```cpp
Configuration& config = Configuration::getInstance();  // Always same instance
```

### 4. Factory Pattern (Command Creation)

**Command Parsing**:
```cpp
// JsonCmdParser creates appropriate command types
auto cmd = JsonCmdParser::parse(jsonString);
// Returns InverterCommand, BatteryCommand, or VirtualBusCmd based on JSON
```

### 5. Strategy Pattern (Logger/Storage)

**Pluggable Implementations**:
```cpp
// Logger strategy
Configuration::setLogger(std::make_shared<SpdLogWrapper>());
// Storage strategy
Configuration::setStorage(std::make_unique<JsonStorage>());
```

---

## Message Flow

### Synchronous Message Flow (Polling)

```
1. Sender calls:
   bus.sendMessage(senderId, message)
   
2. VirtualBus:
   - Acquires lock (mutex)
   - Enqueues message to receiver's queue
   - Notifies condition variable
   - Releases lock

3. Receiver polls:
   while (running_) {
       if (bus.receiveMessage(receiverId, msg)) {
           processMessage(msg);
       }
   }

4. receiveMessage():
   - Acquires lock
   - Dequeues from message queue
   - Releases lock
   - Returns dequeued message
```

### Asynchronous Message Flow (Callback)

```
1. Receiver registers:
   bus.registerCallback(receiverId, callbackFunction)

2. Sender sends:
   bus.sendMessage(senderId, message)

3. VirtualBus:
   - Enqueues message
   - Checks if callback registered
   - Invokes callback (may be on sender's thread or async)

4. Callback handler:
   - Processes message
   - Updates application state
```

### Complete Workflow Diagram

```
┌─────────────────────────────────────────────────────────┐
│                    SendTask Thread                       │
│  1. Create message                                       │
│  2. Call bus.sendMessage()                              │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                    VirtualBus Lock                       │
│  3. Acquire mutex                                        │
│  4. Find receiver in task registry                      │
│  5. Enqueue message to receiver's queue                 │
│  6. Invoke receiver's callback (if registered)          │
│  7. Notify condition variable                           │
│  8. Release mutex                                        │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        ▼                         ▼
┌───────────────────┐    ┌──────────────────────┐
│  ReceiveTask Poll │    │ Callback Invocation  │
│                   │    │                      │
│ 9. Polling loop   │    │ 9. Handler called    │
│ 10. Call receive  │    │ 10. Process message  │
│ 11. Process msg   │    │ 11. Update state     │
└───────────────────┘    └──────────────────────┘
```

---

## Threading Model

### Thread Hierarchy

```
Main Thread
    │
    ├── VirtualBus (main thread)
    │   └── Runs message broker logic
    │
    ├── SendTask Thread
    │   └── Periodically creates and sends messages
    │
    ├── ReceiveTask Thread
    │   └── Processes incoming messages
    │
    ├── ThreadPool Worker Threads (optional)
    │   ├── Worker 1
    │   ├── Worker 2
    │   ├── Worker 3
    │   └── Worker 4
    │
    └── Other Task Threads
        └── Custom application threads
```

### Synchronization Primitives

#### Mutex (std::mutex)

**Usage**: Protects VirtualBus::tasks_ map

```cpp
std::lock_guard<std::mutex> lock(busMutex_);
// Critical section
// Automatic unlock on scope exit
```

#### Condition Variable (std::condition_variable)

**Usage**: Signals message arrival to waiting threads

```cpp
busConditionVariable_.notify_one();  // Wake one waiting thread
busConditionVariable_.wait(lock);    // Wait for notification
```

#### Atomic Flag (std::atomic<bool>)

**Usage**: Thread-safe running state

```cpp
std::atomic<bool> running_;  // No lock needed for read/write
```

### Race Condition Prevention

**Scenario 1: Attach while sending**
```
Thread A: bus.attach(1, "Task1")
Thread B: bus.sendMessage(1, msg)

Solution: Both use mutex lock
```

**Scenario 2: Callback while detaching**
```
Thread A: bus.detach(1)
Thread B: Processing callback for task 1

Solution: Check running_ flag in callback
```

---

## Module Dependencies

### Dependency Graph

```
Application Tasks
    ↓
Task (base class)
    ↓
VirtualBus ← Configuration ← IStorage (JsonStorage)
    ↓              ↓
VirtualBusCmd  ILogger (SpdLogWrapper, StdCoutLogger)
    ↓
InverterCommand, BatteryCommand
```

### Module Relationships

| Module | Depends On | Provides |
|--------|-----------|----------|
| VirtualBus | ILogger, ThreadPool | Message routing |
| Task | VirtualBus, ILogger | Runnable base |
| Configuration | IStorage, ILogger | Config mgmt |
| ErrorHandler | ILogger | Error reporting |
| ThreadPool | - | Worker threads |
| JsonStorage | - | JSON I/O |
| SpdLogWrapper | spdlog lib | Structured logging |

---

## Design Patterns

### 1. Observer Pattern (Callbacks)

**Pattern**: Subject notifies observers of state changes

```cpp
class VirtualBus {
    // Observers (callbacks)
    std::unordered_map<int, CallbackFunction> callbacks_;
    
    // Notify observers
    void sendMessage(...) {
        if (callbacks_[taskId]) {
            callbacks_[taskId](message);  // Notify
        }
    }
};
```

### 2. Chain of Responsibility (Error Handling)

```
Application → ErrorHandler → Logger → Output
```

### 3. Template Method (Task Lifecycle)

```cpp
class Task {
    void start() {
        thread_ = std::thread([this]() { run(); });
    }
    virtual void run() = 0;  // Override in subclass
};
```

### 4. Adapter Pattern (Logger Interface)

```cpp
class SpdLogWrapper : public ILogger {
    // Adapts spdlog library to ILogger interface
};

class StdCoutLogger : public ILogger {
    // Adapts cout to ILogger interface
};
```

### 5. RAII Pattern (Resource Management)

```cpp
std::lock_guard<std::mutex> lock(mutex_);  // Acquires lock
// Use resource
// Automatically releases lock on scope exit
```

---

## State Diagrams

### Task State Machine

```
┌──────────┐
│ Created  │
└────┬─────┘
     │ start()
     ▼
┌──────────┐
│ Running  │◄──────┐
└────┬─────┘       │
     │ stop()      │ (continue running)
     ▼             │
┌──────────┐       │
│ Stopping ├───────┘
└────┬─────┘
     │ join() completes
     ▼
┌──────────┐
│Terminated│
└──────────┘
```

### VirtualBus State Machine

```
┌──────────────┐
│ Constructed  │
└────┬─────────┘
     │ First attach()
     ▼
┌──────────────┐
│   Running    │◄─────────┐
└────┬─────────┘          │
     │ attach/detach/send │
     │    messages        │
     └────────────────────┘
     │ shutdown()
     ▼
┌──────────────┐
│ Shutdown     │
└──────────────┘
```

---

## Performance Considerations

### Time Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| attach() | O(1) | Hash map insertion |
| detach() | O(1) | Hash map deletion |
| sendMessage() | O(1) | Queue enqueue |
| receiveMessage() | O(1) | Queue dequeue |
| registerCallback() | O(1) | Hash map update |

### Space Complexity

- Per task: O(N) where N is message queue size
- Total: O(T × N) where T is number of tasks

### Memory Usage Optimization

1. **Message Pool** (if needed):
   ```cpp
   // Pre-allocate message objects
   std::vector<Message> pool(1000);
   ```

2. **String Interning**:
   ```cpp
   // Reuse task names
   std::string& name = taskRegistry[taskId];
   ```

3. **Callback Optimization**:
   ```cpp
   // Use move semantics for message passing
   bus.sendMessage(id, std::move(message));
   ```

### Latency Considerations

1. **Callback vs. Polling**:
   - Callback: Lower latency (event-driven)
   - Polling: Higher latency but simpler logic

2. **Lock Contention**:
   - High task count → Higher contention
   - Solution: Message queue per task (already implemented)

3. **Thread Scheduling**:
   - More threads → More context switches
   - Balance with number of CPU cores

---

## Extension Points

### 1. Custom Command Types

**Extend VirtualBusCmd**:
```cpp
class SensorCommand : public VirtualBusCmd {
private:
    double sensorValue_;
    
public:
    void setSensorValue(double v) { sensorValue_ = v; }
    double getSensorValue() const { return sensorValue_; }
    
    void print() const override {
        std::cout << "Sensor: " << sensorValue_ << std::endl;
    }
};
```

### 2. Custom Task Types

**Extend Task**:
```cpp
class MyCustomTask : public Task {
protected:
    void run() override {
        // Custom implementation
    }
};
```

### 3. Storage Backends

**Implement IStorage**:
```cpp
class XmlStorage : public IStorage {
    bool load(...) override { /* XML loading */ }
    bool save(...) override { /* XML saving */ }
};
```

### 4. Logger Implementations

**Implement ILogger**:
```cpp
class FileLogger : public ILogger {
    void info(const std::string& msg) override { /* File output */ }
    // Other methods...
};
```

### 5. Command Parsers

**Extend JsonCmdParser**:
```cpp
class XmlCmdParser {
    static std::shared_ptr<VirtualBusCmd> parse(const std::string& xml);
};
```

---

## Best Practices

### 1. Task Design

- Keep run() implementations simple
- Use callbacks for event-driven processing
- Always check `running_` flag

### 2. Message Handling

- Use custom command types for clarity
- Implement print() for debugging
- Update timestamps appropriately

### 3. Thread Safety

- Always use lock_guard with mutex
- Avoid nested locks
- Use condition variables for synchronization

### 4. Configuration

- Load configuration early
- Use sensible defaults
- Validate configuration values

### 5. Error Handling

- Log all errors with appropriate severity
- Propagate errors up the call stack
- Handle timeout scenarios gracefully

---

## Future Enhancements

1. **Message Priority Queue**: Prioritize certain messages
2. **Async Task Execution**: Run multiple tasks per thread
3. **Message Filtering**: Subscribe to specific message types
4. **Performance Metrics**: Built-in profiling and statistics
5. **Remote Bus Communication**: Connect multiple VirtualBus instances
6. **Message History**: Record message flow for debugging
7. **Task Scheduling**: Cron-like task scheduling
8. **Fault Tolerance**: Automatic task recovery

---

## References

- **Concurrency Patterns**: https://en.wikipedia.org/wiki/Message_passing
- **Thread Safety**: https://en.cppreference.com/w/cpp/thread
- **Design Patterns**: https://refactoring.guru/design-patterns
- **AUTOSAR Standard**: https://www.autosar.org/

---

**End of Architecture Documentation**
