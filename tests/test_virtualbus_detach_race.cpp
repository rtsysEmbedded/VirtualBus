// Regression test for a dangling-reference bug in
// VirtualBus::receiveMessage().
//
// The original implementation looked up a task once, then captured a
// reference to its message queue and captured *that reference* by
// reference in the condition_variable::wait() predicate:
//
//     auto& taskInfo = it->second;
//     auto& queue = taskInfo.messageQueue;
//     busConditionVariable_.wait(lock, [&queue, this] {
//         return !queue.empty() || !running_;
//     });
//
// wait() releases busMutex_ while the thread is parked, so a concurrent
// detach() can run in that window, erase the task's unordered_map entry
// (destroying the TaskInfo the reference points into), and leave the
// predicate holding a dangling reference. The next time the predicate is
// evaluated -- which happens as soon as *anything* notifies the condition
// variable, not just a message for this task -- it touches freed memory.
//
// The fix re-looks-up the task by id on every predicate check instead of
// caching a reference across the wait.
//
// This binary is built (see tests/CMakeLists.txt) with its own copy of
// VirtualBus.cpp/ThreadPool.cpp/ErrorHandler.cpp compiled under
// AddressSanitizer, rather than linking against the shared unicore_core
// static library, so the whole call path -- library code included -- is
// instrumented and the use-after-free is reported deterministically
// instead of depending on timing and on the freed memory happening to be
// visibly corrupted.
#include "test_framework.h"

#include "VirtualBus.h"

#include <atomic>
#include <chrono>
#include <thread>

VB_TEST(ReceiveMessage_SurvivesConcurrentDetach) {
    VirtualBus bus;
    bus.attach(42, "Ephemeral");

    std::atomic<bool> receiverStarted{false};
    std::atomic<bool> receiverReturned{false};
    bool result = true;

    std::thread receiverThread([&]() {
        std::shared_ptr<VirtualBusCmd> message;
        receiverStarted = true;
        result = bus.receiveMessage(42, message);
        receiverReturned = true;
    });

    // Give the receiver thread time to reach the condition_variable wait
    // before detaching the task out from under it. This is inherently a
    // race; the sleep just makes the interesting interleaving the
    // overwhelmingly likely one, and ASan/UBSan (enabled in
    // tests/CMakeLists.txt when available) makes the check for the bug
    // itself non-timing-dependent.
    while (!receiverStarted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    bus.detach(42);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Safety net: detach() doesn't itself wake waiters for the detached
    // task (that's a separate, narrower behavior than the memory-safety
    // bug this test targets -- a receiver waiting on a task detached out
    // from under it currently blocks until the bus shuts down). Shut the
    // bus down so the receiver thread is guaranteed to return and this
    // test doesn't hang.
    bus.shutdown();
    receiverThread.join();

    VB_CHECK(receiverReturned.load());
    VB_CHECK(result == false);
}

int main() {
    return vbtest::runAll();
}
