// Regression tests for VirtualBus's bounded per-task message queues and
// overflow handling.
//
// TaskInfo::messageQueues used to be unbounded std::queue instances: a
// receiver that never called receiveMessage() (or a callback that never
// got invoked) could have messages pile up without limit. VirtualBus now
// takes a maxQueueDepth (default VirtualBus::kDefaultMaxQueueDepth) and
// applies it per task, per priority level. The policy is reject-new: a
// message that would exceed the cap is dropped rather than evicting an
// older one or blocking the sender, and sendMessage() reports this via
// ReturnType::BUSY rather than silently succeeding.
#include "test_framework.h"

#include "VirtualBus.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace {

class DummyCmd : public VirtualBusCmd {
public:
    void print() const override {}
};

} // namespace

VB_TEST(SendMessage_TargetedFillsToCapacityThenReturnsBusy) {
    VirtualBus bus(nullptr, nullptr, /*maxQueueDepth=*/3);
    bus.attach(1, "Sender");
    bus.attach(2, "Receiver"); // deliberately never drained in this test

    for (int i = 0; i < 3; ++i) {
        auto cmd = std::make_shared<DummyCmd>();
        VB_CHECK(bus.sendMessage(1, cmd, 2) == ReturnType::OK);
    }

    auto overflowCmd = std::make_shared<DummyCmd>();
    VB_CHECK(bus.sendMessage(1, overflowCmd, 2) == ReturnType::BUSY);

    // Exactly maxQueueDepth messages should be retrievable -- the queue
    // wasn't silently allowed to grow past the cap, and the successful
    // sends weren't corrupted or lost by the rejected one.
    int received = 0;
    std::thread drainer([&]() {
        std::shared_ptr<VirtualBusCmd> msg;
        while (bus.receiveMessage(2, msg)) {
            ++received;
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bus.shutdown();
    drainer.join();
    VB_CHECK(received == 3);
}

VB_TEST(SendMessage_BroadcastPartiallyDeliversWhenOneRecipientIsFull) {
    VirtualBus bus(nullptr, nullptr, /*maxQueueDepth=*/1);
    bus.attach(1, "Sender");
    bus.attach(2, "Full");
    bus.attach(3, "HasRoom");

    // Pre-fill receiver 2's queue to capacity.
    auto fillCmd = std::make_shared<DummyCmd>();
    VB_CHECK(bus.sendMessage(1, fillCmd, 2) == ReturnType::OK);

    // Broadcast: receiver 2 is full (dropped), receiver 3 has room.
    auto broadcastCmd = std::make_shared<DummyCmd>();
    VB_CHECK(bus.sendMessage(1, broadcastCmd) == ReturnType::BUSY);

    // The recipient that had room must still get the message -- one full
    // receiver must not silently block delivery to everyone else.
    std::shared_ptr<VirtualBusCmd> received;
    VB_CHECK(bus.receiveMessage(3, received));
    VB_CHECK(received.get() == broadcastCmd.get());

    bus.shutdown();
}

VB_TEST(SendMessage_ThreadPoolSaturationReportsBusyWithoutCrashing) {
    // maxQueueDepth is deliberately huge (no message-queue send in this
    // test could ever hit it) while threadPoolQueueDepth is tiny: this
    // isolates ThreadPool's own dispatch-queue bound from VirtualBus's
    // per-task message-queue bound (already covered by the two tests
    // above), which share the same default and would otherwise always
    // trip first, since a message never reaches ThreadPool::enqueue() at
    // all unless its per-task queue delivery already succeeded.
    VirtualBus bus(nullptr, nullptr, /*maxQueueDepth=*/1000, /*threadPoolQueueDepth=*/2);
    bus.attach(1, "Sender");
    bus.attach(2, "Receiver");

    // A callback that blocks forever saturates every worker thread (and
    // then the dispatch queue itself) after only a few sends, forcing
    // ThreadPool::enqueue() to throw internally. sendMessage() must
    // catch that and report it as ReturnType::BUSY, not let the
    // exception propagate out and crash the caller -- while the message
    // itself still lands in the per-task queue every time (maxQueueDepth
    // is nowhere near exhausted), retrievable via receiveMessage().
    std::atomic<bool> unblock{false};
    bus.registerCallback(2, [&](std::shared_ptr<VirtualBusCmd>) {
        while (!unblock.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Worst case, every worker thread (hardware_concurrency() of them)
    // picks up and blocks on one dispatch each, plus threadPoolQueueDepth
    // (2) more can queue behind them -- so by send N =
    // hardware_concurrency() + 2 + 1 at the very latest, ThreadPool must
    // be full and enqueue() must throw. 30 sends is comfortably beyond
    // any realistic hardware_concurrency() value.
    bool sawBusy = false;
    for (int i = 0; i < 30; ++i) {
        auto cmd = std::make_shared<DummyCmd>();
        if (bus.sendMessage(1, cmd, 2) == ReturnType::BUSY) {
            sawBusy = true;
        }
    }
    VB_CHECK(sawBusy);

    unblock = true;

    // Even under ThreadPool saturation, every one of the 30 messages
    // should still be sitting in the per-task queue (maxQueueDepth=1000
    // was never in danger), confirming the try/catch around
    // threadPool_.enqueue() doesn't also drop the message itself.
    int received = 0;
    std::thread drainer([&]() {
        std::shared_ptr<VirtualBusCmd> msg;
        while (bus.receiveMessage(2, msg)) {
            ++received;
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    bus.shutdown();
    drainer.join();
    VB_CHECK(received == 30);
}

int main() {
    return vbtest::runAll();
}
