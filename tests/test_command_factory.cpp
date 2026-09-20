// Regression test for CommandFactory, the registry RemoteBridge uses to
// reconstruct a concrete VirtualBusCmd subclass from the CommandType tag
// carried in an incoming envelope.
#include "test_framework.h"

#include "CommandFactory.h"
#include "InverterCommand.h"
#include "BatteryCommand.h"

VB_TEST(CommandFactory_IsRegisteredIsFalseBeforeAnyRegistration) {
    CommandFactory factory;
    VB_CHECK(factory.isRegistered(CommandType::Inverter) == false);
    VB_CHECK(factory.isRegistered(CommandType::Battery) == false);
}

VB_TEST(CommandFactory_CreateReturnsNullptrForUnregisteredType) {
    CommandFactory factory;
    VB_CHECK(factory.create(CommandType::Inverter) == nullptr);
}

VB_TEST(CommandFactory_CreatesTheRegisteredConcreteType) {
    CommandFactory factory;
    factory.registerType(CommandType::Inverter,
                          [](std::shared_ptr<ILogger> logger) { return std::make_shared<InverterCommand>(logger); });

    VB_CHECK(factory.isRegistered(CommandType::Inverter));

    auto cmd = factory.create(CommandType::Inverter);
    VB_CHECK(cmd != nullptr);
    if (cmd) {
        VB_CHECK(cmd->getType() == CommandType::Inverter);
        VB_CHECK(std::dynamic_pointer_cast<InverterCommand>(cmd) != nullptr);
    }
}

VB_TEST(CommandFactory_SupportsMultipleDistinctRegistrations) {
    CommandFactory factory;
    factory.registerType(CommandType::Inverter,
                          [](std::shared_ptr<ILogger> logger) { return std::make_shared<InverterCommand>(logger); });
    factory.registerType(CommandType::Battery,
                          [](std::shared_ptr<ILogger> logger) { return std::make_shared<BatteryStateCmd>(logger); });

    auto inv = factory.create(CommandType::Inverter);
    auto batt = factory.create(CommandType::Battery);
    VB_CHECK(inv != nullptr && std::dynamic_pointer_cast<InverterCommand>(inv) != nullptr);
    VB_CHECK(batt != nullptr && std::dynamic_pointer_cast<BatteryStateCmd>(batt) != nullptr);

    // Still nothing registered for the types this test never registered.
    VB_CHECK(factory.create(CommandType::Gateway) == nullptr);
    VB_CHECK(factory.create(CommandType::Json) == nullptr);
}

VB_TEST(CommandFactory_LaterRegistrationReplacesEarlierOneForSameType) {
    CommandFactory factory;
    int callCount = 0;
    factory.registerType(CommandType::Inverter, [&](std::shared_ptr<ILogger> logger) {
        ++callCount;
        return std::make_shared<InverterCommand>(logger);
    });
    factory.registerType(CommandType::Inverter, [&](std::shared_ptr<ILogger> logger) {
        callCount += 100;
        return std::make_shared<InverterCommand>(logger);
    });

    auto cmd = factory.create(CommandType::Inverter);
    VB_CHECK(cmd != nullptr);
    // Only the second (replacing) creator should have run.
    VB_CHECK(callCount == 100);
}

int main() {
    return vbtest::runAll();
}
