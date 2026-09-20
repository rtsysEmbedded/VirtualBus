#include "LoopbackTransport.h"

std::pair<std::shared_ptr<LoopbackTransport>, std::shared_ptr<LoopbackTransport>> LoopbackTransport::createPair() {
    // std::shared_ptr<T>(new T()) rather than std::make_shared: the
    // constructor is private, and make_shared can't call it from outside
    // the class even via a friend declaration on the aggregate-init form
    // used here, so this is the simplest way to construct two instances
    // and immediately point each at the other.
    std::shared_ptr<LoopbackTransport> a(new LoopbackTransport());
    std::shared_ptr<LoopbackTransport> b(new LoopbackTransport());
    a->peer_ = b;
    b->peer_ = a;
    return {a, b};
}

LoopbackTransport::~LoopbackTransport() {
    stop();
}

bool LoopbackTransport::send(const std::string& bytes) {
    if (!running_) {
        return false;
    }
    auto peer = peer_.lock();
    if (!peer) {
        return false;
    }
    peer->enqueueIncoming(bytes);
    return true;
}

void LoopbackTransport::setReceiveHandler(ReceiveHandler handler) {
    handler_ = std::move(handler);
}

void LoopbackTransport::start() {
    if (running_) {
        return;
    }
    running_ = true;
    worker_ = std::thread(&LoopbackTransport::workerLoop, this);
}

void LoopbackTransport::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void LoopbackTransport::enqueueIncoming(const std::string& bytes) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        incoming_.push(bytes);
    }
    cv_.notify_one();
}

void LoopbackTransport::workerLoop() {
    while (true) {
        std::string bytes;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !incoming_.empty() || !running_; });
            if (!running_ && incoming_.empty()) {
                break;
            }
            bytes = std::move(incoming_.front());
            incoming_.pop();
        }
        if (handler_) {
            // A receive handler is caller-supplied (RemoteBridge's, in
            // practice); an uncaught exception from it would otherwise
            // propagate out of this std::thread's function with no
            // propagation path back to the joining thread, calling
            // std::terminate(). Defense in depth: RemoteBridge already
            // guards its own handler body, but this transport shouldn't
            // rely on every caller doing so correctly.
            try {
                handler_(bytes);
            } catch (...) {
            }
        }
    }
}
