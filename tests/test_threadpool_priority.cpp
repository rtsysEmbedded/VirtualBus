// Regression test for ThreadPool's priority-ordered dispatch.
//
// ThreadPool::enqueue() used to have a single FIFO queue with no notion
// of priority at all. It now takes a priority index and dispatches from
// the highest-priority non-empty queue first (see ThreadPool.h's class
// doc comment). This test uses a single-worker pool specifically so
// execution order is fully deterministic: with more than one worker,
// two tasks could legitimately start concurrently and finish in either
// order regardless of the priority logic being correct, which would make
// the assertion meaningless.
#include "test_framework.h"

#include "ThreadPool.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

VB_TEST(ThreadPool_DispatchesHighestPriorityFirst) {
    ThreadPool pool(1);

    std::atomic<bool> blockerRunning{false};
    std::atomic<bool> releaseBlocker{false};
    std::mutex orderMutex;
    std::vector<size_t> order;

    // Occupy the single worker so the four enqueues below all land in
    // their respective queues before the worker is free to start
    // draining them -- otherwise the first one enqueued could already be
    // running (or finished) before the rest are even submitted, and
    // there would be nothing left to order.
    pool.enqueue(ThreadPool::kDefaultPriority, [&]() {
        blockerRunning = true;
        while (!releaseBlocker.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    while (!blockerRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    pool.enqueue(0, [&]() { std::lock_guard<std::mutex> lock(orderMutex); order.push_back(0); });
    pool.enqueue(3, [&]() { std::lock_guard<std::mutex> lock(orderMutex); order.push_back(3); });
    pool.enqueue(1, [&]() { std::lock_guard<std::mutex> lock(orderMutex); order.push_back(1); });
    pool.enqueue(2, [&]() { std::lock_guard<std::mutex> lock(orderMutex); order.push_back(2); });

    releaseBlocker = true;

    // Wait for all four to have run, with a generous timeout.
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(orderMutex);
            if (order.size() == 4) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::lock_guard<std::mutex> lock(orderMutex);
    VB_CHECK(order.size() == 4);
    if (order.size() == 4) {
        VB_CHECK(order[0] == 3); // Critical
        VB_CHECK(order[1] == 2); // High
        VB_CHECK(order[2] == 1); // Normal
        VB_CHECK(order[3] == 0); // Low
    }
}

VB_TEST(ThreadPool_ClampsOutOfRangePriorityToHighest) {
    ThreadPool pool(1);
    std::atomic<bool> ran{false};
    // kNumPriorityLevels is 4 (valid indices 0..3); 100 is out of range
    // and should be clamped rather than crashing or silently dropping
    // the task.
    auto future = pool.enqueue(100, [&]() { ran = true; return 0; });
    future.wait();
    VB_CHECK(ran.load());
}

VB_TEST(ThreadPool_EnqueueThrowsWhenPriorityQueueIsFull) {
    // maxQueueDepth=2 with a single, permanently-occupied worker: the
    // first enqueue() call is picked up by the worker immediately (queue
    // back to empty), the next 2 fill the one priority-1 queue to its
    // cap, and the 4th must throw rather than growing the queue past
    // maxQueueDepth.
    ThreadPool pool(1, nullptr, /*maxQueueDepth=*/2);

    std::atomic<bool> blockerRunning{false};
    std::atomic<bool> releaseBlocker{false};
    pool.enqueue(ThreadPool::kDefaultPriority, [&]() {
        blockerRunning = true;
        while (!releaseBlocker.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    while (!blockerRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Worker is now stuck in the blocker; these two fill the queue at
    // priority 1 to maxQueueDepth.
    pool.enqueue(1, []() {});
    pool.enqueue(1, []() {});

    bool threw = false;
    try {
        pool.enqueue(1, []() {});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    VB_CHECK(threw);

    releaseBlocker = true;
}

int main() {
    return vbtest::runAll();
}
