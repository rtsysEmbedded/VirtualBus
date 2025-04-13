#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "IStorage.h"
#include "ILogger.h"
#include <memory>
#include <mutex>

/**
 * @brief Singleton class representing a configuration handler that uses an abstract storage and logging system.
 */
class Configuration {
public:
    /**
     * @brief Get the singleton instance of the Configuration class.
     *
     * @return Reference to the singleton Configuration instance.
     */
    static Configuration& getInstance() {
        static Configuration instance;
        return instance;
    }

    // Delete copy constructor and assignment operator to ensure single instance
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;

    /**
     * @brief Set the storage backend for the Configuration class.
     *
     * @param[in] storage A unique pointer to an instance of a storage class.
     */
    void setStorage(std::unique_ptr<IStorage> storage) {
        std::lock_guard<std::mutex> lock(mutex_);
        storage_ = std::move(storage);
    }

    /**
     * @brief Set the logger instance for the Configuration class.
     *
     * @param[in] logger A shared pointer to an instance of a logger.
     */
    void setLogger(std::shared_ptr<ILogger> logger) {
        std::lock_guard<std::mutex> lock(mutex_);
        logger_ = std::move(logger);
    }

    /**
     * @brief Load configuration from a storage medium.
     *
     * @param[in] source Path or identifier for the source.
     * @return True if loading is successful, otherwise false.
     */
    bool load(const std::string& source) {
        if (!storage_) {
            if (logger_) logger_->error("Storage backend is not set.");
            return false;
        }
        return storage_->load(source);
    }

    /**
     * @brief Save configuration to a storage medium.
     *
     * @param[in] destination Path or identifier for the destination.
     * @return True if saved successfully, otherwise false.
     */
    bool save(const std::string& destination) {
        if (!storage_) {
            if (logger_) logger_->error("Storage backend is not set.");
            return false;
        }
        return storage_->save(destination);
    }

    /**
     * @brief Get a configuration value by key.
     *
     * @param[in] key The key of the configuration value.
     * @return The configuration value as a string.
     */
    std::string getConfig(const std::string& key) const {
        if (!storage_) {
            if (logger_) logger_->error("Storage backend is not set.");
            return "";
        }
        return storage_->getValue(key);
    }

    /**
     * @brief Set a configuration value.
     *
     * @param[in] key The key to set.
     * @param[in] value The value to set.
     */
    void setConfig(const std::string& key, const std::string& value) {
        if (storage_) {
            storage_->setValue(key, value);
        } else {
            if (logger_) logger_->error("Storage backend is not set.");
        }
    }

private:
    // Private constructor to prevent instantiation from outside the class
    Configuration() = default;

    std::unique_ptr<IStorage> storage_; ///< Storage instance to manage configuration data
    std::shared_ptr<ILogger> logger_;    ///< Logger instance for logging messages
    mutable std::mutex mutex_;          ///< Mutex to protect access to storage and logger

};

#endif // CONFIGURATION_H
