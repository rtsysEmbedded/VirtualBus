// Regression test for LoopbackTransport, the in-process ITransport used
// to test RemoteBridge deterministically without a real socket.
#include "test_framework.h"

#include "LoopbackTransport.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace {

// Small helper: collects bytes handed to a receive handler, with a
// bounded wait so a broken transport fails the test instead of hanging
// the binary forever.
struct Collector {
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::string> received;

    LoopbackTransport::ReceiveHandler handler() {
        return [this](const std::string& bytes) {
            std::lock_guard<std::mutex> lock(mutex);
            received.push_back(bytes);
            cv.notify_all();
        };
    }

    bool waitFor(size_t n, int ms = 2000) {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, std::chrono::milliseconds(ms), [&] { return received.size() >= n; });
    }
};

} // namespace

VB_TEST(LoopbackTransport_DeliversSendToPeerAsynchronously) {
    auto [a, b] = LoopbackTransport::createPair();
    Collector collectedByB;
    b->setReceiveHandler(collectedByB.handler());
    a->start();
    b->start();

    VB_CHECK(a->send("hello"));
    VB_CHECK(collectedByB.waitFor(1));
    VB_CHECK(collectedByB.received.size() == 1);
    if (!collectedByB.received.empty()) {
        VB_CHECK(collectedByB.received[0] == "hello");
    }

    a->stop();
    b->stop();
}

VB_TEST(LoopbackTransport_PreservesSendOrderPerDirection) {
    auto [a, b] = LoopbackTransport::createPair();
    Collector collectedByB;
    b->setReceiveHandler(collectedByB.handler());
    a->start();
    b->start();

    VB_CHECK(a->send("first"));
    VB_CHECK(a->send("second"));
    VB_CHECK(a->send("third"));
    VB_CHECK(collectedByB.waitFor(3));

    VB_CHECK(collectedByB.received.size() == 3);
    if (collectedByB.received.size() == 3) {
        VB_CHECK(collectedByB.received[0] == "first");
        VB_CHECK(collectedByB.received[1] == "second");
        VB_CHECK(collectedByB.received[2] == "third");
    }

    a->stop();
    b->stop();
}

VB_TEST(LoopbackTransport_IsBidirectional) {
    auto [a, b] = LoopbackTransport::createPair();
    Collector collectedByA, collectedByB;
    a->setReceiveHandler(collectedByA.handler());
    b->setReceiveHandler(collectedByB.handler());
    a->start();
    b->start();

    VB_CHECK(a->send("a-to-b"));
    VB_CHECK(b->send("b-to-a"));
    VB_CHECK(collectedByB.waitFor(1));
    VB_CHECK(collectedByA.waitFor(1));

    VB_CHECK(collectedByB.received.size() == 1 && collectedByB.received[0] == "a-to-b");
    VB_CHECK(collectedByA.received.size() == 1 && collectedByA.received[0] == "b-to-a");

    a->stop();
    b->stop();
}

VB_TEST(LoopbackTransport_SendFailsWhenSenderSideNotStarted) {
    auto [a, b] = LoopbackTransport::createPair();
    Collector collectedByB;
    b->setReceiveHandler(collectedByB.handler());
    b->start();
    // 'a' was never start()ed.
    VB_CHECK(a->send("should not be accepted") == false);

    b->stop();
}

VB_TEST(LoopbackTransport_SendFailsAfterStop) {
    auto [a, b] = LoopbackTransport::createPair();
    a->start();
    b->start();
    VB_CHECK(a->send("before-stop"));
    a->stop();
    VB_CHECK(a->send("after-stop") == false);
    b->stop();
}

VB_TEST(LoopbackTransport_SendFailsWhenPeerDestroyed) {
    std::shared_ptr<LoopbackTransport> a;
    {
        auto pair = LoopbackTransport::createPair();
        a = pair.first; // keep only 'a' alive past this scope
        a->start();
        pair.second->start();
        pair.second->stop();
        // 'pair' (holding the only other shared_ptr to the peer) goes
        // out of scope here, dropping the peer's last reference.
    }
    VB_CHECK(a->send("peer is gone") == false);
    a->stop();
}

int main() {
    return vbtest::runAll();
}
