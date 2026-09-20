#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "IClock.h"
#include "ILogger.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

/**
 * @brief Liveness watchdog: participants (typically Task instances, see
 * Task.h's watchdog_/kickWatchdog()) register with a timeout and must call
 * kick() at least that often; a participant that doesn't gets reported to
 * a caller-supplied handler.
 *
 * This exists because a hung callback or a task stuck in an infinite loop
 * doesn't otherwise announce itself -- see the ThreadPool-saturation
 * scenario this project's own tests found (a callback that never returns
 * can exhaust every worker thread), which is exactly the kind of failure
 * a watchdog is for catching in the first place.
 *
 * Deliberately decoupled from any specific response to a timeout: the
 * default is just a logged error, not an automatic std::terminate() or
 * task restart, because forcibly tearing down the process is not always
 * the right reaction and shouldn't be silently baked into a general-
 * purpose class. Wire in whatever response fits via setTimeoutHandler(),
 * for example escalating through ErrorHandler:
 *
 *     watchdog.setTimeoutHandler([logger](int id, const std::string& name) {
 *         ErrorHandler::handleError("Watchdog", name + " (id " +
 *             std::to_string(id) + ") failed to check in",
 *             ErrorHandler::ErrorSeverity::CRITICAL, logger);
 *     });
 *
 * Takes an IClock (defaults to a real-time SystemClock) so timeout logic
 * can be driven by a VirtualClock in tests without real waiting for the
 * full timeout duration -- though the background monitor thread still
 * polls on real wall-clock time at checkInterval, since a
 * condition_variable has no notion of a virtual clock; see the class's
 * tests for how to work with that.
 */
class Watchdog {
public:
    using TimeoutHandler = std::function<void(int id, const std::string& name)>;

    /**
     * @brief Constructor for Watchdog.
     *
     * @param[in] clock Time source for evaluating timeouts; defaults to a real-time SystemClock.
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     * @param[in] checkInterval How often the background monitor thread wakes up to check for timeouts. Real wall-clock time regardless of `clock`.
     */
    explicit Watchdog(std::shared_ptr<IClock> clock = nullptr,
                       std::shared_ptr<ILogger> logger = nullptr,
                       std::chrono::milliseconds checkInterval = std::chrono::milliseconds(100));

    /**
     * @brief Destructor; stops the monitor thread if still running.
     */
    ~Watchdog();

    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;

    /**
     * @brief Registers (or re-registers) a participant to be monitored.
     *
     * @param[in] id Unique identifier for the participant (e.g. a Task's id).
     * @param[in] name Human-readable name, used in logs and the timeout handler.
     * @param[in] timeout How long this participant may go without calling kick() before it's reported as timed out.
     */
    void registerParticipant(int id, const std::string& name, std::chrono::milliseconds timeout);

    /**
     * @brief Stops monitoring a participant.
     * @param[in] id Identifier previously passed to registerParticipant().
     */
    void unregisterParticipant(int id);

    /**
     * @brief Proves a participant is still alive. Also clears that
     * participant's timed-out flag, so a participant that recovers after
     * being reported can be monitored (and re-reported) again.
     *
     * @param[in] id Identifier previously passed to registerParticipant(). A kick() for an unknown id is silently ignored.
     */
    void kick(int id);

    /**
     * @brief Sets the handler invoked (on the monitor thread, outside any
     * internal lock) when a participant times out. Replaces any
     * previously set handler. Safe to call kick()/registerParticipant()/
     * unregisterParticipant() (including for the timed-out participant
     * itself) from within the handler.
     */
    void setTimeoutHandler(TimeoutHandler handler);

    /**
     * @brief Starts the background monitor thread. No-op if already running.
     */
    void start();

    /**
     * @brief Stops the background monitor thread and joins it. No-op if not running.
     */
    void stop();

private:
    struct ParticipantInfo {
        std::string name;
        uint64_t timeoutMs;
        uint64_t lastKickMs;
        bool timedOutAlready = false;
    };

    void monitorLoop();

    std::shared_ptr<IClock> clock_;
    std::shared_ptr<ILogger> logger_;
    std::chrono::milliseconds checkInterval_;
    TimeoutHandler timeoutHandler_;

    std::unordered_map<int, ParticipantInfo> participants_;
    std::mutex mutex_;
    std::condition_variable cv_; ///< Lets stop() wake the monitor thread promptly instead of waiting out checkInterval_
    std::atomic<bool> running_;
    std::thread monitorThread_;
};

#endif // WATCHDOG_H
