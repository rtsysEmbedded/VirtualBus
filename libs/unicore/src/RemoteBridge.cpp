#include "RemoteBridge.h"
#include "Task.h"

#include "nlohmann/json.hpp"

RemoteBridge::RemoteBridge(VirtualBus& bus, std::shared_ptr<ITransport> transport,
                           std::shared_ptr<CommandFactory> factory, std::shared_ptr<ILogger> logger)
    : bus_(bus), transport_(std::move(transport)), factory_(std::move(factory)), logger_(std::move(logger)),
      bridgeId_(TaskID::getID()) {}

RemoteBridge::~RemoteBridge() {
    stop();
}

void RemoteBridge::start() {
    if (running_) {
        return;
    }
    running_ = true;
    bus_.attach(bridgeId_, "RemoteBridge");
    bus_.registerCallback(bridgeId_, [this](std::shared_ptr<VirtualBusCmd> cmd) { onLocalMessage(cmd); });
    transport_->setReceiveHandler([this](const std::string& bytes) { onRemoteBytes(bytes); });
    transport_->start();
    if (logger_) {
        logger_->info("RemoteBridge: Started (task id " + std::to_string(bridgeId_) + ").");
    }
}

void RemoteBridge::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    transport_->stop();
    bus_.detach(bridgeId_);
    if (logger_) {
        logger_->info("RemoteBridge: Stopped.");
    }
}

void RemoteBridge::onLocalMessage(std::shared_ptr<VirtualBusCmd> cmd) {
    if (!running_ || !cmd) {
        return;
    }
    // A user-overridden serializePayload() is arbitrary code; guard
    // against it throwing so a bad override can't crash whatever thread
    // is delivering the local callback (the caller's sendMessage() path,
    // or a ThreadPool worker for an async callback).
    try {
        nlohmann::json envelope;
        envelope["type"] = static_cast<int>(cmd->getType());
        envelope["priority"] = static_cast<int>(cmd->getPriority());
        envelope["payload"] = cmd->serializePayload();

        if (!transport_->send(envelope.dump()) && logger_) {
            logger_->warn("RemoteBridge: transport rejected an outgoing message (not started/connected).");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->error(std::string("RemoteBridge: failed to serialize outgoing message: ") + e.what());
        }
    }
}

void RemoteBridge::onRemoteBytes(const std::string& bytes) {
    if (!running_) {
        return;
    }
    // Envelopes arrive from a remote peer -- untrusted input from this
    // bridge's point of view even on a friendly link, since it could be
    // truncated, out of protocol version, or simply malformed. Every
    // parse/lookup below is guarded so a single bad envelope is dropped
    // (logged) rather than crashing the transport's receive thread via
    // an uncaught exception (std::thread has no propagation path for
    // that -- it would call std::terminate()).
    try {
        nlohmann::json envelope = nlohmann::json::parse(bytes);

        if (!envelope.contains("type") || !envelope["type"].is_number_integer()) {
            if (logger_) {
                logger_->error("RemoteBridge: received envelope missing/invalid 'type'; dropping.");
            }
            return;
        }
        const auto type = static_cast<CommandType>(envelope["type"].get<int>());

        auto cmd = factory_->create(type, logger_);
        if (!cmd) {
            if (logger_) {
                logger_->error("RemoteBridge: no CommandFactory entry for received CommandType " +
                                std::to_string(envelope["type"].get<int>()) + "; dropping.");
            }
            return;
        }

        if (envelope.contains("priority") && envelope["priority"].is_number_integer()) {
            const int p = envelope["priority"].get<int>();
            if (p >= 0 && p < static_cast<int>(kPriorityLevels)) {
                cmd->setPriority(static_cast<Priority>(p));
            }
        }

        const nlohmann::json payload = envelope.value("payload", nlohmann::json::object());
        if (!cmd->deserializePayload(payload)) {
            if (logger_) {
                logger_->error("RemoteBridge: deserializePayload() rejected the received payload; dropping.");
            }
            return;
        }

        // Broadcast (default targetId) with this bridge as sender: the
        // bridge itself is excluded from its own broadcast, so this
        // re-injection is never handed back to onLocalMessage() -- see
        // the class doc comment for why that's what prevents a
        // remote-forward loop.
        const ReturnType rt = bus_.sendMessage(bridgeId_, cmd);
        if (rt != ReturnType::OK && logger_) {
            logger_->warn("RemoteBridge: local delivery of a remote message did not fully succeed.");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->error(std::string("RemoteBridge: failed to parse/process an incoming envelope: ") + e.what());
        }
    }
}
