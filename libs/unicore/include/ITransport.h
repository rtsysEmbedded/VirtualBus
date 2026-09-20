#ifndef I_TRANSPORT_H
#define I_TRANSPORT_H

#include <functional>
#include <string>

/**
 * @brief Abstract byte-stream transport used by RemoteBridge to forward
 * VirtualBus messages to and from a remote peer.
 *
 * A transport carries opaque, already-serialized message envelopes -- it
 * knows nothing about VirtualBusCmd, CommandType, or priority. This keeps
 * RemoteBridge free to run over anything that can move a length-delimited
 * string to another endpoint: an in-process LoopbackTransport for
 * deterministic tests, a real TcpTransport for two separate processes, or
 * (not implemented here, but the same interface would support it) a
 * message queue, serial link, or CAN transport for an embedded target.
 *
 * Delivery to the receive handler is always asynchronous relative to the
 * sending side's send() call, on an implementation-owned thread -- never
 * invoked synchronously from inside send(). This matters because
 * RemoteBridge::onLocalMessage() calls send() from inside a VirtualBus
 * callback invocation; a transport that delivered synchronously into the
 * peer's receive handler, which itself may call back into a VirtualBus,
 * would risk deep or even mutually-recursive call stacks between two
 * bridged buses. Implementations must not call the handler set by
 * setReceiveHandler() from within send().
 */
class ITransport {
public:
    /// Invoked (on an implementation-owned thread, never from inside
    /// send()) with each opaque envelope received from the peer.
    using ReceiveHandler = std::function<void(const std::string&)>;

    virtual ~ITransport() = default;

    /**
     * @brief Sends an opaque, already-serialized envelope to the peer.
     * @param[in] bytes The envelope to send.
     * @return True if the transport accepted the bytes for delivery
     * (not a delivery guarantee -- e.g. TcpTransport can still fail
     * asynchronously if the peer disconnects), false if the transport
     * isn't currently able to send at all (e.g. not started/connected).
     */
    virtual bool send(const std::string& bytes) = 0;

    /**
     * @brief Sets the handler invoked for every envelope received from
     * the peer. Must be called before start() to guarantee no received
     * envelope is missed; replacing the handler while running is
     * implementation-defined.
     */
    virtual void setReceiveHandler(ReceiveHandler handler) = 0;

    /// Starts the transport (e.g. begins listening/connecting and spins
    /// up its receive thread). Idempotent: calling start() while already
    /// started is a no-op.
    virtual void start() = 0;

    /// Stops the transport and joins any background thread it owns.
    /// Idempotent, and safe to call from the destructor path.
    virtual void stop() = 0;
};

#endif // I_TRANSPORT_H
