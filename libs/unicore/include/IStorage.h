#ifndef I_STORAGE_H
#define I_STORAGE_H

#include <string>

/**
 * @brief Interface representing a generic storage mechanism.
 */
class IStorage {
public:
    virtual ~IStorage() = default;

    /**
     * @brief Load configuration data from a storage medium.
     *
     * @param[in] source Path or identifier for the source.
     * @return True if loading is successful, otherwise false.
     */
    virtual bool load(const std::string& source) = 0;

    /**
     * @brief Save configuration data to a storage medium.
     *
     * @param[in] destination Path or identifier for the destination.
     * @return True if saving is successful, otherwise false.
     */
    virtual bool save(const std::string& destination) = 0;

    /**
     * @brief Get a configuration value by key.
     *
     * @param[in] key The key for the value.
     * @return The value as a string.
     */
    virtual std::string getValue(const std::string& key) const = 0;

    /**
     * @brief Set a configuration value.
     *
     * @param[in] key The key to set.
     * @param[in] value The value to set.
     */
    virtual void setValue(const std::string& key, const std::string& value) = 0;
};

#endif // I_STORAGE_H
