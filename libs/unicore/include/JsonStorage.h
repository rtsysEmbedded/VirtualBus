#ifndef JSON_STORAGE_H
#define JSON_STORAGE_H

#include "IStorage.h"
#include "ILogger.h"
#include <unordered_map>
#include "nlohmann/json.hpp"
#include <fstream>
#include <memory>

/**
 * @brief Class representing a JSON-based storage mechanism.
 */
class JsonStorage : public IStorage {
public:
    /**
     * @brief Constructor for JsonStorage.
     *
     * @param[in] logger A shared pointer to a logger instance for logging messages.
     */
    JsonStorage(std::shared_ptr<ILogger> logger = nullptr)
        : logger_(logger) {}

    /**
     * @brief Load configuration from a JSON file.
     *
     * @param[in] filePath Path to the JSON configuration file.
     * @return True if the configuration is successfully loaded, otherwise false.
     */
    bool load(const std::string& filePath) override {
        try {
            std::ifstream configFile(filePath);
            if (!configFile.is_open()) {
                if (logger_) logger_->error("Failed to open JSON file: " + filePath);
                return false;
            }

            nlohmann::json jsonConfig;
            configFile >> jsonConfig;

            // Parse JSON into internal storage
            for (auto& element : jsonConfig.items()) {
                if (element.value().is_string()) {
                    storage_[element.key()] = element.value().get<std::string>();
                } else {
                    storage_[element.key()] = element.value().dump();
                }
            }

            if (logger_) logger_->info("Data successfully loaded from JSON file: " + filePath);
            return true;
        } catch (const std::exception& e) {
            if (logger_) logger_->error("Exception while loading JSON: " + std::string(e.what()));
            return false;
        }
    }

    /**
     * @brief Save configuration to a JSON file.
     *
     * @param[in] filePath Path to save the JSON file.
     * @return True if saved successfully, otherwise false.
     */
    bool save(const std::string& filePath) override {
        try {
            std::ofstream configFile(filePath);
            if (!configFile.is_open()) {
                if (logger_) logger_->error("Failed to open file for saving: " + filePath);
                return false;
            }

            nlohmann::json jsonConfig(storage_);
            configFile << jsonConfig.dump(4); // Pretty print with indentation

            if (logger_) logger_->info("Data successfully saved to JSON file: " + filePath);
            return true;
        } catch (const std::exception& e) {
            if (logger_) logger_->error("Exception while saving JSON: " + std::string(e.what()));
            return false;
        }
    }

    /**
     * @brief Get a configuration value by key.
     *
     * @param[in] key The key for the value.
     * @return The value as a string.
     */
    std::string getValue(const std::string& key) const override {
        auto it = storage_.find(key);
        if (it != storage_.end()) {
            return it->second;
        }
        if (logger_) logger_->warn("Key '" + key + "' not found in storage");
        return "";
    }

    /**
     * @brief Set a configuration value.
     *
     * @param[in] key The key to set.
     * @param[in] value The value to set.
     */
    void setValue(const std::string& key, const std::string& value) override {
        storage_[key] = value;
        if (logger_) logger_->info("Set value for key '" + key + "': " + value);
    }

    /**
     * @brief Set a logger for the JsonStorage.
     *
     * @param[in] logger A shared pointer to a logger instance.
     */
    void setLogger(std::shared_ptr<ILogger> logger) {
        logger_ = logger;
    }

    /**
     * @brief Validate if a required key exists in the configuration.
     *
     * @param[in] key The required key to validate.
     * @return True if the key exists, otherwise false.
     */
    bool validateKey(const std::string& key) const {
        if (storage_.find(key) == storage_.end()) {
            if (logger_) logger_->error("Validation failed: Key '" + key + "' is missing in storage.");
            return false;
        }
        return true;
    }

private:
    std::unordered_map<std::string, std::string> storage_; ///< Internal storage for configuration data
    std::shared_ptr<ILogger> logger_;                       ///< Logger instance for logging messages
};

#endif // JSON_STORAGE_H
