// Smoke and feature tests covering VirtualBus's attach/send/receive
// contract: broadcast delivery (the original behavior), targeted
// delivery and priority-ordered dequeuing (added afterward), and the
// validation/error-reporting that came with them.
//
// Kept alongside the regression tests so that a future change to
// receiveMessage() (see test_virtualbus_detach_race.cpp) can't quietly
// break the common, non-racy path while fixing the racy one.
#include "test_framework.h"

#include "VirtualBus.h"
#include "VirtualClock.h"

#include <chrono>
#include <thread>

namespace {

class DummyCmd : public VirtualBusCmd {
public:
    void print() const override {}
};

} // namespace

VB_TEST(ReceiveMessage_ReturnsFalse_ForUnknownTask) {
    VirtualBus bus;
    std::shared_ptr<VirtualBusCmd> message;
    VB_CHECK(bus.receiveMessage(999, message) == false);
}

VB_TEST(SendMessage_BroadcastDeliversToEveryOtherAttachedTask) {
    VirtualBus bus;
    bus.attach(1, "Sender");
    bus.attach(2, "ReceiverA");
    bus.attach(3, "ReceiverB");

    auto cmd = std::make_shared<DummyCmd>();
    // No targetId -> VirtualBus::kBroadcast, the default.
    VB_CHECK(bus.sendMessage(1, cmd) == ReturnType::OK);

    std::shared_ptr<VirtualBusCmd> receivedA, receivedB;
    VB_CHECK(bus.receiveMessage(2, receivedA));
    VB_CHECK(receivedA.get() == cmd.get());
    VB_CHECK(bus.receiveMessage(3, receivedB));
    VB_CHECK(receivedB.get() == cmd.get());

    bus.shutdown();
}

VB_TEST(SendMessage_TargetedDeliversOnlyToThatTask) {
    VirtualBus bus;
    bus.attach(1, "Sender");
    bus.attach(2, "Target");
    bus.attach(3, "Bystander");

    auto cmd = std::make_shared<DummyCmd>();
    VB_CHECK(bus.sendMessage(1, cmd, 2) == ReturnType::OK);

    std::shared_ptr<VirtualBusCmd> received;
    VB_CHECK(bus.receiveMessage(2, received));
    VB_CHECK(received.get() == cmd.get());

    // The bystander should have nothing queued: it's still attached, so
    // detach() won't wake it, but shutdown() will -- and only via
    // running_ becoming false, not via a delivered message. Run its
    // receive on a separate thread and confirm it comes back false only
    // after shutdown().
    bool bystanderGotMessage = true;
    std::thread t([&]() {
        std::shared_ptr<VirtualBusCmd> msg;
        bystanderGotMessage = bus.receiveMessage(3, msg);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bus.shutdown();
    t.join();
    VB_CHECK(bystanderGotMessage == false);
}

VB_TEST(SendMessage_TargetedToUnknownTask_ReturnsNotFound) {
    VirtualBus bus;
    bus.attach(1, "Sender");
    auto cmd = std::make_shared<DummyCmd>();
    VB_CHECK(bus.sendMessage(1, cmd, 999) == ReturnType::NOT_FOUND);
    bus.shutdown();
}

VB_TEST(SendMessage_TargetedToSelf_ReturnsInvalidArgument) {
    VirtualBus bus;
    bus.attach(1, "Sender");
    auto cmd = std::make_shared<DummyCmd>();
    VB_CHECK(bus.sendMessage(1, cmd, 1) == ReturnType::INVALID_ARGUMENT);
    bus.shutdown();
}

VB_TEST(SendMessage_FromUnknownSender_ReturnsNotFound) {
    VirtualBus bus;
    bus.attach(2, "Receiver");
    auto cmd = std::make_shared<DummyCmd>();
    VB_CHECK(bus.sendMessage(999, cmd, 2) == ReturnType::NOT_FOUND);
    bus.shutdown();
}

VB_TEST(Attach_RejectsDuplicateTaskId) {
    VirtualBus bus;
    VB_CHECK(bus.attach(1, "First") == ReturnType::OK);
    VB_CHECK(bus.attach(1, "Second") == ReturnType::INVALID_ARGUMENT);
    bus.shutdown();
}

VB_TEST(Attach_RejectsNegativeTaskId) {
    VirtualBus bus;
    // -1 is VirtualBus::kBroadcast; any negative id is reserved.
    VB_CHECK(bus.attach(-1, "Bad") == ReturnType::INVALID_ARGUMENT);
    VB_CHECK(bus.attach(-42, "AlsoBad") == ReturnType::INVALID_ARGUMENT);
    VB_CHECK(bus.attach(0, "Fine") == ReturnType::OK);
    bus.shutdown();
}

VB_TEST(ReceiveMessage_DequeuesHighestPriorityFirst) {
    VirtualBus bus;
    bus.attach(1, "Sender");
    bus.attach(2, "Receiver");

    // Sent in an order that does NOT match priority order, so a plain
    // FIFO queue would hand these back Low, Critical, Normal -- the send
    // order -- rather than priority order.
    auto low = std::make_shared<DummyCmd>();
    low->setPriority(Priority::Low);
    auto critical = std::make_shared<DummyCmd>();
    critical->setPriority(Priority::Critical);
    auto normal = std::make_shared<DummyCmd>();
    normal->setPriority(Priority::Normal);

    VB_CHECK(bus.sendMessage(1, low, 2) == ReturnType::OK);
    VB_CHECK(bus.sendMessage(1, critical, 2) == ReturnType::OK);
    VB_CHECK(bus.sendMessage(1, normal, 2) == ReturnType::OK);

    std::shared_ptr<VirtualBusCmd> first, second, third;
    VB_CHECK(bus.receiveMessage(2, first));
    VB_CHECK(bus.receiveMessage(2, second));
    VB_CHECK(bus.receiveMessage(2, third));

    VB_CHECK(first.get() == critical.get());
    VB_CHECK(second.get() == normal.get());
    VB_CHECK(third.get() == low.get());

    bus.shutdown();
}

VB_TEST(VirtualClock_StampsMessagesWithInjectedTime) {
    auto clock = std::make_shared<VirtualClock>(1000);
    VirtualBus bus(nullptr, clock);
    bus.attach(1, "Sender");
    bus.attach(2, "Receiver");

    clock->set(424242);
    auto cmd = std::make_shared<DummyCmd>();
    VB_CHECK(bus.sendMessage(1, cmd, 2) == ReturnType::OK);

    std::shared_ptr<VirtualBusCmd> received;
    VB_CHECK(bus.receiveMessage(2, received));
    VB_CHECK(received->getTimestamp() == 424242);

    bus.shutdown();
}

int main() {
    return vbtest::runAll();
}
