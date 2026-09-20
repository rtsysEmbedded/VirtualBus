#ifndef REMOTE_BRIDGE_H
#define REMOTE_BRIDGE_H

#include <atomic>
#include <memory>

#include "CommandFactory.h"
#include "ILogger.h"
#include "ITransport.h"
#include "VirtualBus.h"

/**
 * @brief Gateway that attaches to a local VirtualBus as an ordinary task
 * and forwards messages between it and a remote peer's VirtualBus over
 * an ITransport, making VirtualBus's "distributed variant": two (or
 * more, one bridge per remote peer) otherwise-independent buses -- in
 * separate processes, or separate machines over TcpTransport -- kept in
 * sync for the subset of traffic each side chooses to forward.
 *
 * Direction out (local -> remote): RemoteBridge registers a callback for
 * every local broadcast (the same mechanism any other task would use).
 * Since VirtualBus already never delivers a broadcast back to its own
 * sender, this naturally excludes only messages the bridge itself
 * injected -- see the inbound direction below for why that one exclusion
 * is enough to prevent a forwarding loop. Each received local message is
 * serialized into a small JSON envelope, `{"type", "priority",
 * "payload"}` (no sender id: VirtualBus doesn't expose one to callbacks
 * either -- see receiveMessage()/registerCallback() -- so there is
 * nothing meaningful to carry across the wire, and a remote peer's task
 * ids aren't even in this process's id space), and handed to the
 * transport.
 *
 * Direction in (remote -> local): on each envelope received from the
 * transport, RemoteBridge asks its CommandFactory to default-construct
 * the concrete command type named by the envelope's "type", calls
 * deserializePayload() on it, and re-broadcasts it locally via
 * bus.sendMessage(bridgeId, cmd) -- targetId left at its default
 * (broadcast). Using the bridge's own id as sender means the bridge
 * itself never receives its own re-injected message back through its
 * own callback (again, VirtualBus's sender-exclusion), so it never
 * forwards a message it just received from the remote side straight
 * back out to that same side. This is what prevents an infinite bounce
 * between two bridged buses without needing an explicit
 * "already relayed" marker in the envelope -- see RemoteBridge's own
 * test for the topology this reasoning depends on (one bridge per bus;
 * attaching two bridges to the very same bus to represent two peers
 * would defeat it, since each bridge is a distinct task and would
 * happily re-forward what the other just injected).
 *
 * A message whose CommandType has no CommandFactory registration, or
 * whose payload deserializePayload() rejects, is logged and dropped --
 * RemoteBridge never delivers a partially-populated command locally.
 *
 * A local message forwarded out while the transport isn't yet connected
 * (e.g. a TcpTransport connector still retrying its initial connect, or
 * a listener still waiting for accept() to complete) is also dropped,
 * not queued for once the link comes up -- ITransport::send() simply
 * reports failure and onLocalMessage() logs a warning and moves on,
 * consistent with this project's existing reject-new (rather than
 * buffer-and-retry) treatment of anything a receiver isn't currently
 * ready for (see VirtualBus::sendMessage()'s own queue-full handling).
 * A caller that needs every message forwarded once a link comes up
 * should wait for the transport's own "connected" signal (e.g.
 * TcpTransport::isConnected()) before treating the bridge as ready.
 */
class RemoteBridge {
public:
    /**
     * @param[in] bus The local VirtualBus this bridge attaches to.
     * @param[in] transport Carries envelopes to and from the remote
     * peer. Must not already be started; RemoteBridge calls
     * setReceiveHandler() then start() on it from start().
     * @param[in] factory Used to reconstruct concrete command types by
     * CommandType when an envelope arrives from the remote peer. Typically
     * shared with the RemoteBridge (if any) on the peer's own process,
     * since both sides need to agree on which CommandType values mean
     * what -- but each side's factory is consulted only for messages
     * that side receives, so they don't need to be the literal same
     * object, only registered consistently.
     * @param[in] logger Optional logger for dropped/malformed messages
     * and lifecycle events.
     */
    RemoteBridge(VirtualBus& bus, std::shared_ptr<ITransport> transport,
                 std::shared_ptr<CommandFactory> factory, std::shared_ptr<ILogger> logger = nullptr);

    ~RemoteBridge();

    RemoteBridge(const RemoteBridge&) = delete;
    RemoteBridge& operator=(const RemoteBridge&) = delete;

    /// Attaches to the bus, wires up the transport's receive handler, and
    /// starts the transport. Idempotent.
    void start();

    /// Stops the transport and detaches from the bus. Idempotent, safe
    /// to call from the destructor path.
    void stop();

    /// The task id this bridge is (or will be, once start() is called)
    /// attached to the bus under -- useful for a test/caller that wants
    /// to distinguish "came from the bridge" traffic, though ordinary
    /// use never needs it.
    int getBridgeTaskId() const { return bridgeId_; }

private:
    void onLocalMessage(std::shared_ptr<VirtualBusCmd> cmd);
    void onRemoteBytes(const std::string& bytes);

    VirtualBus& bus_;
    std::shared_ptr<ITransport> transport_;
    std::shared_ptr<CommandFactory> factory_;
    std::shared_ptr<ILogger> logger_;
    int bridgeId_;
    std::atomic<bool> running_{false};
};

#endif // REMOTE_BRIDGE_H
