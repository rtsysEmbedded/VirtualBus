#ifndef COMMAND_FACTORY_H
#define COMMAND_FACTORY_H

#include <functional>
#include <memory>
#include <unordered_map>

#include "ILogger.h"
#include "VirtualBusCmd.h"

/**
 * @brief Registry mapping CommandType to a factory function that
 * default-constructs the matching concrete VirtualBusCmd subclass.
 *
 * RemoteBridge needs to reconstruct a concrete command instance (e.g.
 * InverterCommand, BatteryStateCmd) on the receiving side purely from the
 * CommandType tag carried in an envelope, then call deserializePayload()
 * on it -- but libs/unicore (where RemoteBridge and CommandFactory live)
 * has no dependency on and no knowledge of those concrete types, which
 * live in the application layer (src/). The application registers its
 * own command types into an instance of this class at startup and hands
 * that instance to each RemoteBridge, keeping the dependency direction
 * app -> unicore instead of the reverse.
 *
 * Deliberately an explicit, constructed object rather than a global
 * singleton registry: a process-wide static registry would leak
 * registrations across independent tests (or independent buses) sharing
 * one process, making test outcomes depend on execution order.
 */
class CommandFactory {
public:
    /// Constructs a default (unpopulated) instance of the concrete
    /// command type this creator is registered for, forwarding the given
    /// logger to its constructor.
    using Creator = std::function<std::shared_ptr<VirtualBusCmd>(std::shared_ptr<ILogger>)>;

    /// Registers (or replaces) the creator for `type`.
    void registerType(CommandType type, Creator creator) {
        creators_[type] = std::move(creator);
    }

    /// @return True if a creator is registered for `type`.
    bool isRegistered(CommandType type) const {
        return creators_.find(type) != creators_.end();
    }

    /// @return A freshly default-constructed instance of the concrete
    /// type registered for `type`, or nullptr if none is registered.
    std::shared_ptr<VirtualBusCmd> create(CommandType type, std::shared_ptr<ILogger> logger = nullptr) const {
        auto it = creators_.find(type);
        if (it == creators_.end()) {
            return nullptr;
        }
        return it->second(logger);
    }

private:
    std::unordered_map<CommandType, Creator> creators_;
};

#endif // COMMAND_FACTORY_H
