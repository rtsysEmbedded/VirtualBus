// Regression test for RemoteBridge: two independent VirtualBus instances,
// each with its own RemoteBridge, connected by a transport -- the
// "distributed variant". Covers both directions of forwarding, the
// no-infinite-loop guarantee (see RemoteBridge.h's class doc comment for
// why sender-exclusion-from-broadcast is what prevents it), graceful
// dropping of a CommandType the receiving side's CommandFactory doesn't
// know about, and one end-to-end scenario over a real TcpTransport
// (rather than only the deterministic in-process LoopbackTransport) to
// prove the bridge genuinely works across a real socket, not just
// in-process.
#include "test_framework.h"

#include "VirtualBus.h"
#include "RemoteBridge.h"
#include "LoopbackTransport.h"
#include "TcpTransport.h"
#include "CommandFactory.h"
#include "InverterCommand.h"
#include "BatteryCommand.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace {

struct Collector {
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::shared_ptr<VirtualBusCmd>> received;

    VirtualBus::CallbackFunction callback() {
        return [this](std::shared_ptr<VirtualBusCmd> cmd) {
            std::lock_guard<std::mutex> lock(mutex);
            received.push_back(cmd);
            cv.notify_all();
        };
    }

    bool waitFor(size_t n, int ms = 2000) {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, std::chrono::milliseconds(ms), [&] { return received.size() >= n; });
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex);
        return received.size();
    }
};

std::shared_ptr<CommandFactory> makeFullFactory() {
    auto factory = std::make_shared<CommandFactory>();
    factory->registerType(CommandType::Inverter,
                           [](std::shared_ptr<ILogger> logger) { return std::make_shared<InverterCommand>(logger); });
    factory->registerType(CommandType::Battery,
                           [](std::shared_ptr<ILogger> logger) { return std::make_shared<BatteryStateCmd>(logger); });
    return factory;
}

} // namespace

VB_TEST(RemoteBridge_ForwardsLocalBroadcastToRemoteBusInBothDirections) {
    VirtualBus busA;
    VirtualBus busB;
    auto [transportA, transportB] = LoopbackTransport::createPair();
    auto factoryA = makeFullFactory();
    auto factoryB = makeFullFactory();

    RemoteBridge bridgeA(busA, transportA, factoryA);
    RemoteBridge bridgeB(busB, transportB, factoryB);
    bridgeA.start();
    bridgeB.start();

    const int taskA = 100, taskB = 200;
    busA.attach(taskA, "TaskA");
    busB.attach(taskB, "TaskB");

    Collector collectedByB, collectedByA;
    busB.registerCallback(taskB, collectedByB.callback());
    busA.registerCallback(taskA, collectedByA.callback());

    // A -> B: InverterCommand
    auto invCmd = std::make_shared<InverterCommand>();
    invCmd->setVoltage(380.0);
    invCmd->setCurrent(5.5);
    invCmd->setMode(InverterCommand::Mode::Discharging);
    invCmd->setPriority(Priority::High);
    VB_CHECK(busA.sendMessage(taskA, invCmd) == ReturnType::OK);

    VB_CHECK(collectedByB.waitFor(1));
    VB_CHECK(collectedByB.size() == 1);
    if (collectedByB.size() == 1) {
        auto got = std::dynamic_pointer_cast<InverterCommand>(collectedByB.received[0]);
        VB_CHECK(got != nullptr);
        if (got) {
            VB_CHECK(got->getVoltage() == 380.0);
            VB_CHECK(got->getCurrent() == 5.5);
            VB_CHECK(got->getMode() == InverterCommand::Mode::Discharging);
            VB_CHECK(got->getPriority() == Priority::High);
            VB_CHECK(got.get() != invCmd.get()); // crossed the transport, not the same object
        }
    }

    // B -> A: BatteryStateCmd
    auto battCmd = std::make_shared<BatteryStateCmd>();
    battCmd->setMinVoltage(49000);
    battCmd->setMaxVoltage(55000);
    battCmd->setMeanSOC(5000);
    VB_CHECK(busB.sendMessage(taskB, battCmd) == ReturnType::OK);

    VB_CHECK(collectedByA.waitFor(1));
    VB_CHECK(collectedByA.size() == 1);
    if (collectedByA.size() == 1) {
        auto got = std::dynamic_pointer_cast<BatteryStateCmd>(collectedByA.received[0]);
        VB_CHECK(got != nullptr);
        if (got) {
            VB_CHECK(got->getVoltageMinimum() == 49000);
            VB_CHECK(got->getVoltageMaximum() == 55000);
            VB_CHECK(got->getSOCMean() == 5000);
        }
    }

    bridgeA.stop();
    bridgeB.stop();
    busA.shutdown();
    busB.shutdown();
}

VB_TEST(RemoteBridge_DoesNotInfiniteLoopBetweenTwoBridgedBuses) {
    // The critical safety property: one message sent on one side must
    // settle after exactly one delivery on the other side, never
    // bouncing back and forth. See RemoteBridge.h's class doc comment
    // for the sender-exclusion reasoning this depends on.
    VirtualBus busA;
    VirtualBus busB;
    auto [transportA, transportB] = LoopbackTransport::createPair();
    auto factoryA = makeFullFactory();
    auto factoryB = makeFullFactory();

    RemoteBridge bridgeA(busA, transportA, factoryA);
    RemoteBridge bridgeB(busB, transportB, factoryB);
    bridgeA.start();
    bridgeB.start();

    const int taskA = 100, taskB = 200;
    busA.attach(taskA, "TaskA");
    busB.attach(taskB, "TaskB");

    Collector collectedByB, collectedByA;
    busB.registerCallback(taskB, collectedByB.callback());
    busA.registerCallback(taskA, collectedByA.callback());

    auto cmd = std::make_shared<InverterCommand>();
    cmd->setVoltage(1.0);
    VB_CHECK(busA.sendMessage(taskA, cmd) == ReturnType::OK);
    VB_CHECK(collectedByB.waitFor(1));

    // Let anything that would bounce have plenty of time to do so.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    VB_CHECK(collectedByB.size() == 1);
    VB_CHECK(collectedByA.size() == 0);

    bridgeA.stop();
    bridgeB.stop();
    busA.shutdown();
    busB.shutdown();
}

VB_TEST(RemoteBridge_DropsMessageOfCommandTypeReceiverCannotReconstruct) {
    VirtualBus busA;
    VirtualBus busB;
    auto [transportA, transportB] = LoopbackTransport::createPair();
    auto factoryA = makeFullFactory();

    // B's factory only knows Inverter -- simulating a peer running an
    // older/different build that doesn't understand a Battery envelope.
    auto factoryB = std::make_shared<CommandFactory>();
    factoryB->registerType(CommandType::Inverter,
                            [](std::shared_ptr<ILogger> logger) { return std::make_shared<InverterCommand>(logger); });

    RemoteBridge bridgeA(busA, transportA, factoryA);
    RemoteBridge bridgeB(busB, transportB, factoryB);
    bridgeA.start();
    bridgeB.start();

    const int taskA = 100, taskB = 200;
    busA.attach(taskA, "TaskA");
    busB.attach(taskB, "TaskB");

    Collector collectedByB;
    busB.registerCallback(taskB, collectedByB.callback());

    auto battCmd = std::make_shared<BatteryStateCmd>();
    battCmd->setMinVoltage(49000);
    VB_CHECK(busA.sendMessage(taskA, battCmd) == ReturnType::OK);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    VB_CHECK(collectedByB.size() == 0);

    // The bridge must still work afterward for a type B DOES know about
    // -- one dropped message must not wedge the bridge.
    auto invCmd = std::make_shared<InverterCommand>();
    invCmd->setVoltage(111.0);
    VB_CHECK(busA.sendMessage(taskA, invCmd) == ReturnType::OK);
    VB_CHECK(collectedByB.waitFor(1));
    VB_CHECK(collectedByB.size() == 1);
    if (collectedByB.size() == 1) {
        auto got = std::dynamic_pointer_cast<InverterCommand>(collectedByB.received[0]);
        VB_CHECK(got != nullptr);
        if (got) {
            VB_CHECK(got->getVoltage() == 111.0);
        }
    }

    bridgeA.stop();
    bridgeB.stop();
    busA.shutdown();
    busB.shutdown();
}

VB_TEST(RemoteBridge_WorksOverARealTcpTransportNotJustInProcess) {
    VirtualBus busA;
    VirtualBus busB;

    auto listener = TcpTransport::createListener(0);
    listener->start();
    uint16_t port = 0;
    for (int i = 0; i < 200 && port == 0; ++i) {
        port = listener->getBoundPort();
        if (port == 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    VB_CHECK(port != 0);
    if (port == 0) {
        listener->stop();
        return;
    }
    auto connector = TcpTransport::createConnector("127.0.0.1", port);

    auto factoryA = makeFullFactory();
    auto factoryB = makeFullFactory();
    RemoteBridge bridgeA(busA, connector, factoryA); // A connects out
    RemoteBridge bridgeB(busB, listener, factoryB);  // B listens
    bridgeB.start();
    bridgeA.start();

    for (int i = 0; i < 300 && !(listener->isConnected() && connector->isConnected()); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    VB_CHECK(listener->isConnected());
    VB_CHECK(connector->isConnected());

    const int taskA = 1, taskB = 2;
    busA.attach(taskA, "TaskA");
    busB.attach(taskB, "TaskB");

    Collector collectedByB;
    busB.registerCallback(taskB, collectedByB.callback());

    auto cmd = std::make_shared<InverterCommand>();
    cmd->setVoltage(230.0);
    cmd->setCurrent(9.9);
    VB_CHECK(busA.sendMessage(taskA, cmd) == ReturnType::OK);

    VB_CHECK(collectedByB.waitFor(1, 3000));
    VB_CHECK(collectedByB.size() == 1);
    if (collectedByB.size() == 1) {
        auto got = std::dynamic_pointer_cast<InverterCommand>(collectedByB.received[0]);
        VB_CHECK(got != nullptr);
        if (got) {
            VB_CHECK(got->getVoltage() == 230.0);
            VB_CHECK(got->getCurrent() == 9.9);
        }
    }

    bridgeA.stop();
    bridgeB.stop();
    busA.shutdown();
    busB.shutdown();
}

int main() {
    return vbtest::runAll();
}
