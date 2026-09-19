#ifndef VIRTUAL_CLOCK_H
#define VIRTUAL_CLOCK_H

#include "IClock.h"
#include <atomic>

/**
 * @brief IClock whose value is set explicitly rather than tracking real
 * time. Inject this into VirtualBus (via its constructor) to get
 * deterministic, controllable message timestamps in tests and
 * benchmarks -- and, eventually, to drive a record/replay engine against
 * a recorded time sequence instead of real wall-clock time.
 *
 * Thread-safe: advance()/set()/nowMs() may be called from any thread
 * while VirtualBus is running.
 */
class VirtualClock : public IClock {
public:
    explicit VirtualClock(uint64_t startMs = 0) : nowMs_(startMs) {}

    uint64_t nowMs() const override {
        return nowMs_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Advances the clock forward by the given number of milliseconds.
     */
    void advance(uint64_t deltaMs) {
        nowMs_.fetch_add(deltaMs, std::memory_order_relaxed);
    }

    /**
     * @brief Sets the clock to an absolute value.
     */
    void set(uint64_t ms) {
        nowMs_.store(ms, std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> nowMs_;
};

#endif // VIRTUAL_CLOCK_H
