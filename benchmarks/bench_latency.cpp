// Latency/jitter benchmark for VirtualBus::sendMessage() -> callback
// dispatch.
//
// This is a measurement tool, not a correctness test (see tests/ for
// those). It exists to make the cost of the current architecture visible
// and comparable across changes and commits:
//
//   - "Baseline": raw sendMessage() -> callback latency with a single
//     receiver, to establish a floor.
//   - "Fan-out scaling": VirtualBus::sendMessage() broadcasts every
//     message to every attached task's queue and invokes every
//     registered callback, all under one global busMutex_ (see
//     libs/unicore/src/VirtualBus.cpp). This section shows how latency
//     grows with the number of attached receivers.
//   - "Logging on the hot path": logger_->info(...) calls are synchronous
//     and sit directly in sendMessage()/receiveMessage(). This section
//     isolates that cost from the fan-out cost above.
//   - "Fan-out + logging combined": the realistic worst case of both
//     effects at once.
//   - "Paced send": messages sent on a sleep_for()-paced loop (closer to
//     how SendTask/ReceiveTask in src/ actually behave) instead of a
//     tight busy loop, to show that periodic wakeup itself adds jitter
//     independent of anything VirtualBus does.
//
// Latency is measured with steady_clock (effectively nanosecond
// resolution on this platform), independent of VirtualBusCmd's own
// timestamp_ field, which is only millisecond-resolution and far too
// coarse to see any of the effects above.
//
// Usage: build with `cmake --build . --target bench_latency` (see
// benchmarks/CMakeLists.txt for why this target pins its own -O2 rather
// than following the top-level CMAKE_BUILD_TYPE), then run
// ./benchmarks/bench_latency. Results are numbers-only on stdout so a run
// can be piped straight into a file for benchmarks/RESULTS.md.
#include "VirtualBus.h"
#include "StdCoutLogger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <thread>
#include <vector>

#ifndef VB_BENCH_GIT_COMMIT
#define VB_BENCH_GIT_COMMIT "unknown"
#endif

using Clock = std::chrono::steady_clock;

struct TimedCmd : public VirtualBusCmd {
    Clock::time_point sentAt;
    void print() const override {}
};

struct Stats {
    long n = 0;
    double meanUs = 0, p50Us = 0, p90Us = 0, p99Us = 0, maxUs = 0, minUs = 0, stddevUs = 0;
};

static Stats computeStats(std::vector<double>& samplesUs) {
    Stats s;
    s.n = static_cast<long>(samplesUs.size());
    if (s.n == 0) {
        return s;
    }
    std::sort(samplesUs.begin(), samplesUs.end());
    s.minUs = samplesUs.front();
    s.maxUs = samplesUs.back();
    s.p50Us = samplesUs[static_cast<size_t>(0.50 * (s.n - 1))];
    s.p90Us = samplesUs[static_cast<size_t>(0.90 * (s.n - 1))];
    s.p99Us = samplesUs[static_cast<size_t>(0.99 * (s.n - 1))];
    double sum = 0;
    for (double v : samplesUs) {
        sum += v;
    }
    s.meanUs = sum / s.n;
    double sq = 0;
    for (double v : samplesUs) {
        sq += (v - s.meanUs) * (v - s.meanUs);
    }
    s.stddevUs = std::sqrt(sq / s.n);
    return s;
}

static void printStats(const std::string& label, const Stats& s) {
    std::printf("%-40s n=%-6ld min=%8.1f  p50=%8.1f  p90=%8.1f  p99=%8.1f  max=%10.1f  mean=%8.1f  stddev=%8.1f  (us)\n",
                label.c_str(), s.n, s.minUs, s.p50Us, s.p90Us, s.p99Us, s.maxUs, s.meanUs, s.stddevUs);
}

// Runs one sender broadcasting `numMessages`, with `numReceivers`
// attached receivers each recording per-message latency via its own
// callback. Returns latency stats pooled across all receivers: with
// broadcast semantics every receiver sees every message, so the pooled
// distribution is what any given receiver actually experiences.
static Stats runScenario(int numReceivers, int numMessages, bool withLogger, bool busySend) {
    auto logger = withLogger ? std::make_shared<StdCoutLogger>() : nullptr;
    VirtualBus bus(logger);

    const int senderId = 0;
    bus.attach(senderId, "Sender");

    std::vector<std::unique_ptr<std::atomic<int>>> receivedCounts;
    std::mutex samplesMutex;
    std::vector<double> samplesUs;
    samplesUs.reserve(static_cast<size_t>(numReceivers) * static_cast<size_t>(numMessages));

    for (int r = 0; r < numReceivers; ++r) {
        int id = 100 + r;
        bus.attach(id, "Receiver" + std::to_string(r));
        receivedCounts.push_back(std::make_unique<std::atomic<int>>(0));
        std::atomic<int>* counter = receivedCounts.back().get();

        bus.registerCallback(id, [&samplesMutex, &samplesUs, counter](std::shared_ptr<VirtualBusCmd> msg) {
            auto now = Clock::now();
            auto* timed = static_cast<TimedCmd*>(msg.get());
            double latencyUs = std::chrono::duration<double, std::micro>(now - timed->sentAt).count();
            {
                std::lock_guard<std::mutex> lock(samplesMutex);
                samplesUs.push_back(latencyUs);
            }
            counter->fetch_add(1, std::memory_order_relaxed);
        });
    }

    for (int i = 0; i < numMessages; ++i) {
        auto cmd = std::make_shared<TimedCmd>();
        cmd->sentAt = Clock::now();
        bus.sendMessage(senderId, cmd);
        if (!busySend) {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }

    // Wait for every receiver to have processed every message, with a
    // generous timeout as a safety net against a hang masking as a slow
    // benchmark.
    auto deadline = Clock::now() + std::chrono::seconds(30);
    for (auto& c : receivedCounts) {
        while (c->load(std::memory_order_relaxed) < numMessages && Clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    bus.shutdown();

    return computeStats(samplesUs);
}

int main() {
    std::time_t now = std::time(nullptr);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S UTC", std::gmtime(&now));

    std::printf("VirtualBus latency/jitter benchmark\n");
    std::printf("commit: %s\n", VB_BENCH_GIT_COMMIT);
    std::printf("run at: %s\n", timeBuf);
    std::printf("hardware_concurrency (ThreadPool size): %u\n\n", std::thread::hardware_concurrency());

    std::printf("=== A. Baseline: 1 sender -> 1 receiver, no logger, busy-send ===\n");
    printStats("1 receiver, 2000 msgs, no logger", runScenario(1, 2000, false, true));

    std::printf("\n=== B. Fan-out scaling: broadcast-to-all cost, no logger, busy-send ===\n");
    printStats("1 receiver,  1000 msgs", runScenario(1, 1000, false, true));
    printStats("5 receivers, 1000 msgs", runScenario(5, 1000, false, true));
    printStats("20 receivers, 1000 msgs", runScenario(20, 1000, false, true));
    printStats("50 receivers, 1000 msgs", runScenario(50, 1000, false, true));

    std::printf("\n=== C. Logging on the hot path: 1 receiver, 1000 msgs, busy-send ===\n");
    printStats("no logger", runScenario(1, 1000, false, true));
    printStats("StdCoutLogger enabled", runScenario(1, 1000, true, true));

    std::printf("\n=== D. Fan-out + logging combined worst case, busy-send ===\n");
    printStats("20 receivers, 1000 msgs, no logger", runScenario(20, 1000, false, true));
    printStats("20 receivers, 1000 msgs, w/ logger", runScenario(20, 1000, true, true));

    std::printf("\n=== E. Paced send (~200us between sends, 1 receiver) ===\n");
    printStats("1 receiver, 1000 msgs, paced", runScenario(1, 1000, false, false));
    printStats("20 receivers, 1000 msgs, paced", runScenario(20, 1000, false, false));

    return 0;
}
