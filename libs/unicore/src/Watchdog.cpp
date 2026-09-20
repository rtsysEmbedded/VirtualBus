#include "Watchdog.h"
#include "SystemClock.h"

#include <vector>

Watchdog::Watchdog(std::shared_ptr<IClock> clock, std::shared_ptr<ILogger> logger,
                   std::chrono::milliseconds checkInterval)
    : clock_(clock ? std::move(clock) : std::make_shared<SystemClock>()),
      logger_(logger), checkInterval_(checkInterval), running_(false) {}

Watchdog::~Watchdog() {
    stop();
}

void Watchdog::registerParticipant(int id, const std::string& name, std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    participants_[id] = ParticipantInfo{name, static_cast<uint64_t>(timeout.count()), clock_->nowMs(), false};
    if (logger_) {
        logger_->info("Watchdog: Registered participant " + name + " (id " + std::to_string(id) +
                      "), timeout " + std::to_string(timeout.count()) + "ms.");
    }
}

void Watchdog::unregisterParticipant(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    participants_.erase(id);
}

void Watchdog::kick(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = participants_.find(id);
    if (it != participants_.end()) {
        it->second.lastKickMs = clock_->nowMs();
        it->second.timedOutAlready = false;
    }
}

void Watchdog::setTimeoutHandler(TimeoutHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeoutHandler_ = std::move(handler);
}

void Watchdog::start() {
    if (running_) {
        return;
    }
    running_ = true;
    monitorThread_ = std::thread(&Watchdog::monitorLoop, this);
    if (logger_) {
        logger_->info("Watchdog: Started.");
    }
}

void Watchdog::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
    }
    cv_.notify_all();
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
    if (logger_) {
        logger_->info("Watchdog: Stopped.");
    }
}

void Watchdog::monitorLoop() {
    while (true) {
        // Collect timed-out participants while holding the lock, then
        // invoke the handler for each afterward with the lock released.
        // Calling the handler while iterating participants_ directly
        // would let a handler that calls back into kick()/register/
        // unregister (an explicitly supported, reasonable thing for it
        // to do) invalidate the iterator mid-loop.
        std::vector<std::pair<int, std::string>> timedOut;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, checkInterval_, [this] { return !running_.load(); });
            if (!running_) {
                break;
            }
            uint64_t now = clock_->nowMs();
            for (auto& [id, info] : participants_) {
                if (!info.timedOutAlready && now - info.lastKickMs > info.timeoutMs) {
                    info.timedOutAlready = true;
                    timedOut.emplace_back(id, info.name);
                }
            }
        }

        for (auto& [id, name] : timedOut) {
            if (logger_) {
                logger_->error("Watchdog: Participant " + name + " (id " + std::to_string(id) + ") timed out.");
            }
            TimeoutHandler handler;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                handler = timeoutHandler_;
            }
            if (handler) {
                handler(id, name);
            }
        }
    }
}
