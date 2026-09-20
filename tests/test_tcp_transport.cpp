// Regression test for TcpTransport, the real point-to-point ITransport
// RemoteBridge uses to bridge two VirtualBus instances across separate
// processes/machines. Uses real POSIX sockets over 127.0.0.1 -- an
// ephemeral (port 0) listener so the test never collides with anything
// already bound on a fixed port.
#include "test_framework.h"

#include "TcpTransport.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Collector {
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::string> received;

    TcpTransport::ReceiveHandler handler() {
        return [this](const std::string& bytes) {
            std::lock_guard<std::mutex> lock(mutex);
            received.push_back(bytes);
            cv.notify_all();
        };
    }

    bool waitFor(size_t n, int ms = 3000) {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, std::chrono::milliseconds(ms), [&] { return received.size() >= n; });
    }
};

// Waits (bounded) for a freshly-started listener to report its actual
// bound ephemeral port. Returns 0 if it never does.
uint16_t waitForBoundPort(TcpTransport& listener, int ms = 2000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (std::chrono::steady_clock::now() < deadline) {
        uint16_t port = listener.getBoundPort();
        if (port != 0) {
            return port;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}

bool waitForBothConnected(TcpTransport& a, TcpTransport& b, int ms = 3000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (std::chrono::steady_clock::now() < deadline) {
        if (a.isConnected() && b.isConnected()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

} // namespace

VB_TEST(TcpTransport_ConnectorReachesEphemeralPortListener) {
    auto listener = TcpTransport::createListener(0);
    listener->start();
    uint16_t port = waitForBoundPort(*listener);
    VB_CHECK(port != 0);
    if (port == 0) {
        listener->stop();
        return;
    }

    auto connector = TcpTransport::createConnector("127.0.0.1", port);
    connector->start();

    VB_CHECK(waitForBothConnected(*listener, *connector));

    connector->stop();
    listener->stop();
}

VB_TEST(TcpTransport_RoundTripsMessagesInBothDirections) {
    auto listener = TcpTransport::createListener(0);
    listener->start();
    uint16_t port = waitForBoundPort(*listener);
    VB_CHECK(port != 0);
    if (port == 0) {
        listener->stop();
        return;
    }

    Collector collectedByListener, collectedByConnector;
    listener->setReceiveHandler(collectedByListener.handler());

    auto connector = TcpTransport::createConnector("127.0.0.1", port);
    connector->setReceiveHandler(collectedByConnector.handler());
    connector->start();

    VB_CHECK(waitForBothConnected(*listener, *connector));

    VB_CHECK(connector->send("ping-1"));
    VB_CHECK(connector->send("ping-2"));
    VB_CHECK(listener->send("pong"));

    VB_CHECK(collectedByListener.waitFor(2));
    VB_CHECK(collectedByConnector.waitFor(1));

    VB_CHECK(collectedByListener.received.size() == 2);
    if (collectedByListener.received.size() == 2) {
        VB_CHECK(collectedByListener.received[0] == "ping-1");
        VB_CHECK(collectedByListener.received[1] == "ping-2");
    }
    VB_CHECK(collectedByConnector.received.size() == 1);
    if (!collectedByConnector.received.empty()) {
        VB_CHECK(collectedByConnector.received[0] == "pong");
    }

    connector->stop();
    listener->stop();
}

VB_TEST(TcpTransport_FramesAPayloadLargerThanOneReadBuffer) {
    // Exercises the length-prefix framing across many recv() calls, not
    // just a single one that happens to contain the whole envelope.
    auto listener = TcpTransport::createListener(0);
    listener->start();
    uint16_t port = waitForBoundPort(*listener);
    VB_CHECK(port != 0);
    if (port == 0) {
        listener->stop();
        return;
    }

    Collector collectedByListener;
    listener->setReceiveHandler(collectedByListener.handler());

    auto connector = TcpTransport::createConnector("127.0.0.1", port);
    connector->start();
    VB_CHECK(waitForBothConnected(*listener, *connector));

    const std::string big(500000, 'x');
    VB_CHECK(connector->send(big));
    VB_CHECK(collectedByListener.waitFor(1));
    VB_CHECK(collectedByListener.received.size() == 1);
    if (!collectedByListener.received.empty()) {
        VB_CHECK(collectedByListener.received[0].size() == big.size());
        VB_CHECK(collectedByListener.received[0] == big);
    }

    connector->stop();
    listener->stop();
}

VB_TEST(TcpTransport_SendFailsBeforeConnectedAndAfterStop) {
    auto listener = TcpTransport::createListener(0);
    listener->start();
    uint16_t port = waitForBoundPort(*listener);
    VB_CHECK(port != 0);
    if (port == 0) {
        listener->stop();
        return;
    }

    auto connector = TcpTransport::createConnector("127.0.0.1", port);
    // Not started yet: must not be connected, must not accept a send().
    VB_CHECK(connector->isConnected() == false);
    VB_CHECK(connector->send("too-early") == false);

    connector->start();
    VB_CHECK(waitForBothConnected(*listener, *connector));
    VB_CHECK(connector->send("now-it-works"));

    connector->stop();
    VB_CHECK(connector->send("after-stop") == false);

    listener->stop();
}

int main() {
    return vbtest::runAll();
}
