#ifndef TCP_TRANSPORT_H
#define TCP_TRANSPORT_H

#ifdef _WIN32
#error "TcpTransport currently supports POSIX sockets only (see TcpTransport.cpp)."
#endif

#include "ILogger.h"
#include "ITransport.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

/**
 * @brief Real point-to-point ITransport over a TCP socket, for
 * RemoteBridge running across two separate processes (or machines).
 *
 * One side listens (createListener()), the other connects
 * (createConnector()) -- a single peer-to-peer link, matching
 * RemoteBridge's one-bridge-per-remote-peer design (see RemoteBridge.h);
 * this is not a multi-peer server. Framing is a 4-byte big-endian length
 * prefix followed by that many payload bytes, so multiple envelopes sent
 * back-to-back on the underlying byte stream are correctly split back
 * into the same discrete `send()` calls on the receiving side.
 *
 * All socket I/O (accept/connect/read) happens on a single background
 * thread started by start(); stop() unblocks it by shutting down and
 * closing whichever file descriptor it may be blocked on, then joins it.
 */
class TcpTransport : public ITransport {
public:
    /// Listens on `port` (0 requests an OS-assigned ephemeral port --
    /// see getBoundPort()) and accepts exactly one incoming connection.
    static std::shared_ptr<TcpTransport> createListener(uint16_t port, std::shared_ptr<ILogger> logger = nullptr);

    /// Connects to `host`:`port`, retrying at a fixed interval until
    /// connected or stop() is called (so it can be start()ed before its
    /// listening peer is ready).
    static std::shared_ptr<TcpTransport> createConnector(const std::string& host, uint16_t port,
                                                          std::shared_ptr<ILogger> logger = nullptr);

    ~TcpTransport() override;

    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;

    /// @return False (without sending) if not yet connected, or if the
    /// underlying write fails (e.g. the peer closed the connection).
    bool send(const std::string& bytes) override;

    void setReceiveHandler(ReceiveHandler handler) override;

    void start() override;
    void stop() override;

    /// True once accept()/connect() has succeeded and the read loop is
    /// running.
    bool isConnected() const { return connected_; }

    /// The actual bound listening port once start() has bound the
    /// socket (meaningful for a listener created with port 0); 0 before
    /// that, or for a connector.
    uint16_t getBoundPort() const { return boundPort_; }

private:
    enum class Role { Listen, Connect };

    TcpTransport(Role role, std::string host, uint16_t port, std::shared_ptr<ILogger> logger);

    void ioThreadMain();
    bool readLoop();
    bool readExact(int fd, void* buffer, size_t length);
    void closeFdLocked(int& fd);

    Role role_;
    std::string host_;
    uint16_t port_;
    std::shared_ptr<ILogger> logger_;

    std::mutex fdMutex_;
    int listenFd_ = -1;
    int connFd_ = -1;

    std::thread ioThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::atomic<uint16_t> boundPort_{0};

    std::mutex sendMutex_;
    ReceiveHandler handler_;
};

#endif // TCP_TRANSPORT_H
