#ifndef I_CLOCK_H
#define I_CLOCK_H

#include <cstdint>

/**
 * @brief Interface for a time source, injectable into VirtualBus.
 *
 * VirtualBus stamps every message with the time it entered the bus
 * (see VirtualBus::sendMessage()) via whichever IClock it was
 * constructed with. Swapping SystemClock for VirtualClock lets a test
 * or benchmark control that time explicitly instead of depending on
 * real wall-clock time, and is the foundation a future deterministic
 * record/replay engine would be built on: replaying a recorded sequence
 * of messages requires driving them against a clock you control, not
 * the system clock.
 */
class IClock {
public:
    virtual ~IClock() = default;

    /**
     * @brief Current time, in milliseconds since an implementation-defined
     * epoch. Only meaningful for computing deltas against other calls to
     * the same IClock instance -- not guaranteed to correspond to wall-clock
     * time (VirtualClock's doesn't).
     */
    virtual uint64_t nowMs() const = 0;
};

#endif // I_CLOCK_H
