// Regression tests for Watchdog and its integration into Task.
//
// Completes the last open Phase 1 roadmap item (Watchdog integration).
// This exists because a hung callback or a task stuck in a loop doesn't
// otherwise announce itself -- see test_virtualbus_overflow.cpp's
// ThreadPool-saturation test, which is exactly the kind of failure a
// watchdog is meant to catch (a task that never returns from a unit of
// work exhausts worker threads with no visible signal otherwise).
//
// Uses VirtualClock (see VirtualClock.h) to drive timeout *thresholds*
// deterministically -- advancing 10 minutes of "time" is instant, no real
// waiting required. The background monitor thread still polls on real
// wall-clock time (a condition_variable has no notion of an injected
// clock), so tests use a short real checkInterval and a short real sleep
// to give it a chance to notice, but never wait out an actual timeout
// duration.
#include "test_framework.h"

#include "Task.h"
#include "VirtualBus.h"
#include "VirtualClock.h"
#include "Watchdog.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace {

// Poll interval used throughout: short enough for fast tests, long
// enough that a handful of iterations comfortably fit inside the sleeps
// below.
constexpr auto kFastCheckInterval = std::chrono::milliseconds(10);
constexpr auto kSettleSleep = std::chrono::milliseconds(60);

} // namespace

VB_TEST(Watchdog_ReportsExactlyOnceWhenNotKicked) {
    auto clock = std::make_shared<VirtualClock>(0);
    Watchdog wd(clock, nullptr, kFastCheckInterval);

    std::atomic<int> timeoutCount{0};
    std::atomic<int> reportedId{-1};
    wd.setTimeoutHandler([&](int id, const std::string&) {
        timeoutCount++;
        reportedId = id;
    });
    wd.start();

    wd.registerParticipant(7, "Hung", std::chrono::milliseconds(500));
    clock->advance(600);
    std::this_thread::sleep_for(kSettleSleep);
    VB_CHECK(timeoutCount.load() == 1);
    VB_CHECK(reportedId.load() == 7);

    // Advancing further without kicking must not re-fire for the same
    // still-timed-out participant.
    clock->advance(1000);
    std::this_thread::sleep_for(kSettleSleep);
    VB_CHECK(timeoutCount.load() == 1);

    wd.stop();
}

VB_TEST(Watchdog_KickedParticipantNeverTimesOut) {
    auto clock = std::make_shared<VirtualClock>(0);
    Watchdog wd(clock, nullptr, kFastCheckInterval);

    std::atomic<int> timeoutCount{0};
    wd.setTimeoutHandler([&](int, const std::string&) { timeoutCount++; });
    wd.start();

    wd.registerParticipant(1, "Healthy", std::chrono::milliseconds(300));
    for (int i = 0; i < 5; ++i) {
        clock->advance(100); // well within the 300ms timeout each step
        wd.kick(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    VB_CHECK(timeoutCount.load() == 0);

    wd.stop();
}

VB_TEST(Watchdog_RecoversAfterKickThenCanTimeOutAgain) {
    auto clock = std::make_shared<VirtualClock>(0);
    Watchdog wd(clock, nullptr, kFastCheckInterval);

    std::atomic<int> timeoutCount{0};
    wd.setTimeoutHandler([&](int, const std::string&) { timeoutCount++; });
    wd.start();

    wd.registerParticipant(2, "Flaky", std::chrono::milliseconds(200));
    clock->advance(300);
    std::this_thread::sleep_for(kSettleSleep);
    VB_CHECK(timeoutCount.load() == 1);

    wd.kick(2); // recovers
    clock->advance(300);
    std::this_thread::sleep_for(kSettleSleep);
    VB_CHECK(timeoutCount.load() == 2);

    wd.stop();
}

VB_TEST(Watchdog_UnregisteredParticipantIsNotMonitored) {
    auto clock = std::make_shared<VirtualClock>(0);
    Watchdog wd(clock, nullptr, kFastCheckInterval);

    std::atomic<int> timeoutCount{0};
    wd.setTimeoutHandler([&](int, const std::string&) { timeoutCount++; });
    wd.start();

    wd.registerParticipant(3, "Removed", std::chrono::milliseconds(100));
    wd.unregisterParticipant(3);
    clock->advance(10000);
    std::this_thread::sleep_for(kSettleSleep);
    VB_CHECK(timeoutCount.load() == 0);

    wd.stop();
}

VB_TEST(Watchdog_KickForUnknownIdIsIgnoredNotCrash) {
    auto clock = std::make_shared<VirtualClock>(0);
    Watchdog wd(clock);
    wd.kick(999); // never registered
    // Reaching here without crashing/throwing is the assertion.
    VB_CHECK(true);
}

// --- Task integration ---

namespace {

class KickingTask : public Task {
public:
    using Task::Task;
    std::atomic<int> iterations{0};

protected:
    void run() override {
        while (running_) {
            kickWatchdog();
            ++iterations;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
};

class HangingTask : public Task {
public:
    using Task::Task;

protected:
    void run() override {
        // Never calls kickWatchdog(): simulates a task stuck on its
        // first unit of work.
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
};

} // namespace

VB_TEST(Task_ThatKicksRegularly_IsNotReportedByWatchdog) {
    VirtualBus bus;

    auto clock = std::make_shared<VirtualClock>(0);
    auto wd = std::make_shared<Watchdog>(clock, nullptr, kFastCheckInterval);
    std::atomic<int> timeoutCount{0};
    wd->setTimeoutHandler([&](int, const std::string&) { timeoutCount++; });
    wd->start();

    // A short timeout (50ms), advanced past in total (10 * 30ms = 300ms)
    // -- this only stays under the timeout at every individual check
    // because KickingTask's own loop (real time, every ~5ms) keeps
    // calling kickWatchdog() in between each advance, repeatedly
    // resetting the "time since last kick" clock. If kickWatchdog() were
    // a no-op, the very first advance would already exceed the 50ms
    // timeout from registration, since nothing would ever move
    // lastKickMs away from its initial value. A single large gap
    // (like the SendTask_ThatKicksRegularly variant used before this
    // rewrite) can't tell the difference between "kicking is wired up"
    // and "kicking is broken but the test window is short."
    KickingTask task("Kicker", bus, nullptr, wd, std::chrono::milliseconds(50));
    task.start();

    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        clock->advance(30);
    }
    std::this_thread::sleep_for(kSettleSleep);

    VB_CHECK(timeoutCount.load() == 0);
    VB_CHECK(task.iterations.load() > 0);

    task.stop();
    wd->stop();
}

VB_TEST(Task_ThatNeverKicks_IsReportedByWatchdogOnceTimeoutElapses) {
    VirtualBus bus;

    auto clock = std::make_shared<VirtualClock>(0);
    auto wd = std::make_shared<Watchdog>(clock, nullptr, kFastCheckInterval);
    std::atomic<int> timeoutCount{0};
    std::atomic<int> reportedId{-1};
    wd->setTimeoutHandler([&](int id, const std::string&) {
        timeoutCount++;
        reportedId = id;
    });
    wd->start();

    HangingTask task("Hanger", bus, nullptr, wd, std::chrono::milliseconds(300));
    task.start();
    int taskId = task.getID();

    clock->advance(400); // past the 300ms timeout, no kick ever happened
    std::this_thread::sleep_for(kSettleSleep);

    VB_CHECK(timeoutCount.load() == 1);
    VB_CHECK(reportedId.load() == taskId);

    task.stop();
    wd->stop();
}

VB_TEST(Task_StopUnregistersFromWatchdog) {
    VirtualBus bus;

    auto clock = std::make_shared<VirtualClock>(0);
    auto wd = std::make_shared<Watchdog>(clock, nullptr, kFastCheckInterval);
    std::atomic<int> timeoutCount{0};
    wd->setTimeoutHandler([&](int, const std::string&) { timeoutCount++; });
    wd->start();

    HangingTask task("Hanger", bus, nullptr, wd, std::chrono::milliseconds(300));
    task.start();
    task.stop(); // unregisters before ever timing out

    clock->advance(10000);
    std::this_thread::sleep_for(kSettleSleep);
    VB_CHECK(timeoutCount.load() == 0);

    wd->stop();
}

int main() {
    return vbtest::runAll();
}
