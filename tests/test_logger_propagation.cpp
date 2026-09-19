// Regression test for the logger-shadowing bug in SendTask/ReciveTask, and
// the matching bug in InverterCommand/VirtualBusCmd.
//
// SendTask and ReceiveTask each used to declare their own private
// `logger_` member with the same name as Task's own (then-private)
// `logger_` member. The subclass copy shadowed the base one, was
// default-constructed to nullptr, and was never assigned from the
// constructor argument -- so every `if (logger_) logger_->info(...)`
// inside SendTask::run() and ReceiveTask::start()/onMessageReceived()
// silently did nothing, no matter what logger the caller passed in.
// That's also exactly the pattern the project's own usage docs show users
// how to copy, so any task written by following the README inherited the
// same bug.
//
// Task::logger_ is now protected and the subclasses no longer declare
// their own copy, so this exercises the real SendTask/ReceiveTask classes
// from src/ and checks the logger actually passed in by the caller is the
// one that receives the log messages.
//
// InverterCommand had the identical shadowing pattern one level down, in
// VirtualBusCmd: InverterCommand declared its own private `logger_` and
// constructed its base with `VirtualBusCmd()` -- no logger argument at
// all -- so every base-class method (updateTimestamp(), setParser(),
// parse(), printBase(), the destructor) always logged through a null
// logger_ regardless of what was passed to InverterCommand's own
// constructor. VirtualBusCmd::logger_ is now protected, InverterCommand
// no longer shadows it, and its constructor forwards the logger to the
// base class.
#include "test_framework.h"

#include "ILogger.h"
#include "InverterCommand.h"
#include "ReciveTask.h"
#include "SendTask.h"
#include "VirtualBus.h"

#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

class RecordingLogger : public ILogger {
public:
    void info(const std::string& message) override { record(message); }
    void warn(const std::string& message) override { record(message); }
    void error(const std::string& message) override { record(message); }
    void critical(const std::string& message) override { record(message); }

    bool anyContains(const std::string& needle) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& message : messages_) {
            if (message.find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

private:
    void record(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back(message);
    }

    mutable std::mutex mutex_;
    std::vector<std::string> messages_;
};

} // namespace

VB_TEST(ReceiveTask_LogsThroughInjectedLogger) {
    auto logger = std::make_shared<RecordingLogger>();
    VirtualBus bus(logger);
    ReceiveTask receiver("Receiver", bus, logger);
    bus.attach(receiver.getID(), receiver.getName());

    // ReceiveTask::start() logs "Callback registered and task started."
    // synchronously, before spawning the worker thread, so this assertion
    // needs no sleep to be deterministic.
    receiver.start();
    VB_CHECK(logger->anyContains("ReceiveTask: Callback registered and task started."));

    receiver.stop();
    receiver.join();
}

VB_TEST(SendTask_LogsThroughInjectedLogger) {
    auto logger = std::make_shared<RecordingLogger>();
    VirtualBus bus(logger);
    SendTask sender("Sender", bus, logger);
    bus.attach(sender.getID(), sender.getName());

    sender.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    sender.stop();
    sender.join();

    VB_CHECK(logger->anyContains("SendTask: Sent InverterCommand"));
}

VB_TEST(InverterCommand_PropagatesLoggerToBaseClass) {
    auto logger = std::make_shared<RecordingLogger>();
    InverterCommand cmd(logger);

    // VirtualBusCmd's constructor logs through the base class's own
    // logger_ member. Before the fix this was always dropped for an
    // InverterCommand: its shadow `logger_` member did get the logger
    // assigned, but the base was constructed with `VirtualBusCmd()` --
    // no logger passed through at all.
    VB_CHECK(logger->anyContains("VirtualBusCmd: Command created with default type Json."));

    // InverterCommand's own constructor logs through the same identifier,
    // which already worked before the fix (its shadow copy was correctly
    // assigned) and must keep working now that the identifier resolves to
    // the inherited member instead.
    VB_CHECK(logger->anyContains("InverterCommand: Initialized with mode Charging."));
}

int main() {
    return vbtest::runAll();
}
