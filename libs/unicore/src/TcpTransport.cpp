#include "TcpTransport.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace {
constexpr int kListenBacklog = 1;
constexpr int kConnectRetryMs = 100;
// Sanity bound on a received frame's declared length, so a garbled or
// malicious stream can't make readLoop() try to allocate an enormous
// buffer before any content has even been validated.
constexpr uint32_t kMaxFrameBytes = 16u * 1024u * 1024u;
} // namespace

std::shared_ptr<TcpTransport> TcpTransport::createListener(uint16_t port, std::shared_ptr<ILogger> logger) {
    return std::shared_ptr<TcpTransport>(new TcpTransport(Role::Listen, "", port, std::move(logger)));
}

std::shared_ptr<TcpTransport> TcpTransport::createConnector(const std::string& host, uint16_t port,
                                                             std::shared_ptr<ILogger> logger) {
    return std::shared_ptr<TcpTransport>(new TcpTransport(Role::Connect, host, port, std::move(logger)));
}

TcpTransport::TcpTransport(Role role, std::string host, uint16_t port, std::shared_ptr<ILogger> logger)
    : role_(role), host_(std::move(host)), port_(port), logger_(std::move(logger)) {}

TcpTransport::~TcpTransport() {
    stop();
}

void TcpTransport::setReceiveHandler(ReceiveHandler handler) {
    handler_ = std::move(handler);
}

void TcpTransport::start() {
    if (running_) {
        return;
    }
    running_ = true;
    ioThread_ = std::thread(&TcpTransport::ioThreadMain, this);
}

void TcpTransport::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    {
        // shutdown()+close() unblock whichever blocking syscall
        // (accept()/connect()/recv()) the io thread may currently be
        // parked in; closeFdLocked() is a no-op for an fd that's
        // already -1 (e.g. a connector still mid-retry-loop, which
        // simply observes running_ == false on its next iteration).
        std::lock_guard<std::mutex> lock(fdMutex_);
        closeFdLocked(listenFd_);
        closeFdLocked(connFd_);
    }
    if (ioThread_.joinable()) {
        ioThread_.join();
    }
    connected_ = false;
}

void TcpTransport::closeFdLocked(int& fd) {
    if (fd >= 0) {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
        fd = -1;
    }
}

bool TcpTransport::send(const std::string& bytes) {
    if (!connected_) {
        return false;
    }
    if (bytes.size() > kMaxFrameBytes) {
        if (logger_) {
            logger_->error("TcpTransport: refusing to send an oversized frame (" + std::to_string(bytes.size()) +
                            " bytes).");
        }
        return false;
    }

    std::lock_guard<std::mutex> lock(sendMutex_);
    int fd;
    {
        std::lock_guard<std::mutex> fdLock(fdMutex_);
        fd = connFd_;
    }
    if (fd < 0) {
        return false;
    }

    uint32_t lengthNetworkOrder = htonl(static_cast<uint32_t>(bytes.size()));
    std::string frame(reinterpret_cast<const char*>(&lengthNetworkOrder), sizeof(lengthNetworkOrder));
    frame += bytes;

    size_t totalSent = 0;
    while (totalSent < frame.size()) {
        ssize_t sent = ::send(fd, frame.data() + totalSent, frame.size() - totalSent, 0);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (logger_) {
                logger_->error(std::string("TcpTransport: send() failed: ") + std::strerror(errno));
            }
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}

void TcpTransport::ioThreadMain() {
    if (role_ == Role::Listen) {
        int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listenFd < 0) {
            if (logger_) logger_->error(std::string("TcpTransport: socket() failed: ") + std::strerror(errno));
            running_ = false;
            return;
        }
        int reuse = 1;
        ::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);

        if (::bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            if (logger_) logger_->error(std::string("TcpTransport: bind() failed: ") + std::strerror(errno));
            ::close(listenFd);
            running_ = false;
            return;
        }

        socklen_t addrLen = sizeof(addr);
        if (::getsockname(listenFd, reinterpret_cast<sockaddr*>(&addr), &addrLen) == 0) {
            boundPort_ = ntohs(addr.sin_port);
        }

        if (::listen(listenFd, kListenBacklog) < 0) {
            if (logger_) logger_->error(std::string("TcpTransport: listen() failed: ") + std::strerror(errno));
            ::close(listenFd);
            running_ = false;
            return;
        }

        {
            std::lock_guard<std::mutex> lock(fdMutex_);
            if (!running_) { // stop() raced us before we got here
                ::close(listenFd);
                return;
            }
            listenFd_ = listenFd;
        }

        int acceptedFd = ::accept(listenFd, nullptr, nullptr);

        {
            std::lock_guard<std::mutex> lock(fdMutex_);
            closeFdLocked(listenFd_); // done listening either way, one peer is enough
        }

        if (acceptedFd < 0) {
            // Either a real accept() error, or stop() closed listenFd_
            // out from under us -- either way, nothing to serve.
            running_ = false;
            return;
        }

        {
            std::lock_guard<std::mutex> lock(fdMutex_);
            if (!running_) {
                ::shutdown(acceptedFd, SHUT_RDWR);
                ::close(acceptedFd);
                return;
            }
            connFd_ = acceptedFd;
        }
        connected_ = true;
    } else { // Role::Connect
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        const std::string portStr = std::to_string(port_);

        while (running_) {
            addrinfo* resolved = nullptr;
            int gaiResult = ::getaddrinfo(host_.c_str(), portStr.c_str(), &hints, &resolved);
            int fd = -1;
            if (gaiResult == 0 && resolved) {
                fd = ::socket(resolved->ai_family, resolved->ai_socktype, resolved->ai_protocol);
                if (fd >= 0 && ::connect(fd, resolved->ai_addr, resolved->ai_addrlen) == 0) {
                    ::freeaddrinfo(resolved);
                    std::lock_guard<std::mutex> lock(fdMutex_);
                    if (!running_) {
                        ::close(fd);
                        return;
                    }
                    connFd_ = fd;
                    connected_ = true;
                    break;
                }
                if (fd >= 0) {
                    ::close(fd);
                }
            }
            if (resolved) {
                ::freeaddrinfo(resolved);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kConnectRetryMs));
        }
        if (!running_) {
            return;
        }
    }

    readLoop();
}

bool TcpTransport::readExact(int fd, void* buffer, size_t length) {
    size_t totalRead = 0;
    char* out = static_cast<char*>(buffer);
    while (totalRead < length) {
        ssize_t bytesRead = ::recv(fd, out + totalRead, length - totalRead, 0);
        if (bytesRead == 0) {
            return false; // peer closed the connection
        }
        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        totalRead += static_cast<size_t>(bytesRead);
    }
    return true;
}

bool TcpTransport::readLoop() {
    int fd;
    {
        std::lock_guard<std::mutex> lock(fdMutex_);
        fd = connFd_;
    }

    while (running_ && fd >= 0) {
        uint32_t lengthNetworkOrder = 0;
        if (!readExact(fd, &lengthNetworkOrder, sizeof(lengthNetworkOrder))) {
            break;
        }
        uint32_t length = ntohl(lengthNetworkOrder);
        if (length > kMaxFrameBytes) {
            if (logger_) {
                logger_->error("TcpTransport: peer declared an oversized frame (" + std::to_string(length) +
                                " bytes); closing connection.");
            }
            break;
        }

        std::string payload(length, '\0');
        if (length > 0 && !readExact(fd, payload.data(), length)) {
            break;
        }

        if (handler_) {
            // See LoopbackTransport::workerLoop()'s identical guard: a
            // receive handler is caller-supplied (RemoteBridge's, in
            // practice) and must not be able to crash this io thread.
            try {
                handler_(payload);
            } catch (...) {
            }
        }
    }

    connected_ = false;
    {
        std::lock_guard<std::mutex> lock(fdMutex_);
        closeFdLocked(connFd_);
    }
    return true;
}
