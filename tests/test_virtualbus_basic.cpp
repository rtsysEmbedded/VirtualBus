// Smoke tests covering VirtualBus's basic attach/send/receive contract.
//
// Kept alongside the regression tests so that a future change to
// receiveMessage() (see test_virtualbus_detach_race.cpp) can't quietly
// break the common, non-racy path while fixing the racy one.
#include "test_framework.h"

#include "VirtualBus.h"

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

VB_TEST(SendMessage_DeliversToOtherAttachedTask) {
    VirtualBus bus;
    bus.attach(1, "Sender");
    bus.attach(2, "Receiver");

    auto cmd = std::make_shared<DummyCmd>();
    bus.sendMessage(1, cmd);

    std::shared_ptr<VirtualBusCmd> received;
    VB_CHECK(bus.receiveMessage(2, received));
    VB_CHECK(received.get() == cmd.get());

    bus.shutdown();
}

VB_TEST(Attach_RejectsDuplicateTaskId) {
    VirtualBus bus;
    VB_CHECK(bus.attach(1, "First") == ReturnType::OK);
    VB_CHECK(bus.attach(1, "Second") == ReturnType::INVALID_ARGUMENT);
    bus.shutdown();
}

int main() {
    return vbtest::runAll();
}
