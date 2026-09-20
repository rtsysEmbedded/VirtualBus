#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include "IClock.h"
#include <chrono>

/**
 * @brief IClock backed by std::chrono::system_clock. This is VirtualBus's
 * default clock when none is injected -- behaves like real wall-clock
 * time, matching what VirtualBusCmd::updateTimestamp() always did before
 * IClock existed.
 */
class SystemClock : public IClock {
public:
    uint64_t nowMs() const override {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return static_cast<uint64_t>(ms.count());
    }
};

#endif // SYSTEM_CLOCK_H
