#ifndef LOOPBACK_TRANSPORT_H
#define LOOPBACK_TRANSPORT_H

#include "ITransport.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

/**
 * @brief In-process ITransport connecting exactly two endpoints, for
 * deterministic tests of RemoteBridge without opening a real socket.
 *
 * Two LoopbackTransport instances are always created together via
 * createPair(), each already pointing at the other as its peer; there is
 * no separate "connect" step. send() on one instance enqueues the bytes
 * onto the other's incoming queue; each instance's own background thread
 * (started by start()) drains its queue and invokes its own receive
 * handler -- so, as ITransport's contract requires, delivery is always
 * asynchronous relative to the sender's send() call, never a direct call
 * from inside it.
 */
class LoopbackTransport : public ITransport {
public:
    /// Creates two LoopbackTransport instances already paired with each
    /// other. Held as shared_ptr because each instance keeps only a
    /// weak_ptr to its peer (avoiding a reference cycle that would leak
    /// both), so something must keep the pair alive.
    static std::pair<std::shared_ptr<LoopbackTransport>, std::shared_ptr<LoopbackTransport>> createPair();

    ~LoopbackTransport() override;

    LoopbackTransport(const LoopbackTransport&) = delete;
    LoopbackTransport& operator=(const LoopbackTransport&) = delete;

    /// Enqueues `bytes` on the peer's incoming queue. Returns false
    /// (without enqueuing) if the peer no longer exists or this side
    /// hasn't been start()ed.
    bool send(const std::string& bytes) override;

    void setReceiveHandler(ReceiveHandler handler) override;

    void start() override;
    void stop() override;

private:
    LoopbackTransport() = default;

    /// Called by the peer's send(): pushes onto this instance's own
    /// incoming queue and wakes its worker thread. Private, but callable
    /// from the peer because both sides are the same class.
    void enqueueIncoming(const std::string& bytes);

    void workerLoop();

    std::weak_ptr<LoopbackTransport> peer_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::string> incoming_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    ReceiveHandler handler_;
};

#endif // LOOPBACK_TRANSPORT_H
