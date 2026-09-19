# VirtualBus Configuration Guide

Complete documentation of VirtualBus configuration options and management.

## Table of Contents

1. [Configuration Overview](#configuration-overview)
2. [Configuration File Format](#configuration-file-format)
3. [Core Settings](#core-settings)
4. [Advanced Settings](#advanced-settings)
5. [Environment Variables](#environment-variables)
6. [Configuration Examples](#configuration-examples)
7. [Configuration API](#configuration-api)
8. [Best Practices](#best-practices)
9. [Troubleshooting](#troubleshooting)

---

## Configuration Overview

### Configuration System Architecture

VirtualBus uses a **flexible configuration system** with multiple backends:

```
Application Code
        ↓
Configuration Singleton
        ↓
IStorage Interface
    ↙          ↖
JsonStorage  XmlStorage (future)
    ↓            ↓
File System
```

### Features

- **Dynamic Loading**: Load configuration at runtime
- **Singleton Pattern**: Single configuration instance
- **Multiple Backends**: Support for different storage formats
- **Type Safety**: Validated configuration values
- **Hot Reload**: Update configuration without restart (optional)

### Configuration Sources (Priority Order)

1. **Command Line Arguments** (highest priority)
2. **Environment Variables**
3. **Configuration File** (default: config.json)
4. **Built-in Defaults** (lowest priority)

---

## Configuration File Format

### JSON Format (Default)

The default configuration file is in JSON format: `config.json`

```json
{
    "log_level": "info",
    "max_threads": "4",
    "mqtt_broker": "mqtt://localhost:1883",
    "mqtt_client_id": "virtualbus_client",
    "mqtt_username": "user",
    "mqtt_password": "password",
    "task_timeout": "5000",
    "enable_watchdog": true,
    "watchdog_timeout": "30000",
    "database_host": "localhost",
    "database_port": "5432",
    "database_name": "virtualbus",
    "cache_enabled": true,
    "cache_ttl": "3600"
}
```

### File Location

**Search Order**:
1. Path specified in code: `config.load("custom/path.json")`
2. Current working directory: `./config.json`
3. Application directory: `<app_dir>/config.json`
4. System config: `/etc/virtualbus/config.json` (Linux)

### File Permissions

**Recommended**:
- Owner: Read/Write
- Group: Read only
- Others: None (600)

```bash
chmod 600 config.json
```

---

## Core Settings

### Logging Configuration

#### log_level

**Description**: Logging verbosity level

**Values**:
- `debug` - Detailed debugging information
- `info` - General information messages
- `warn` - Warning messages
- `error` - Error messages
- `critical` - Critical errors only
- `off` - Logging disabled

**Default**: `info`

**Example**:
```json
{
    "log_level": "debug"
}
```

**Usage**:
```cpp
Configuration& config = Configuration::getInstance();
std::string level = config.getConfig("log_level");

if (level == "debug") {
    logger->debug("Debug enabled");
}
```

#### log_file

**Description**: Output log file path (spdlog only)

**Default**: `logs/virtualbus.log`

**Example**:
```json
{
    "log_file": "logs/application.log"
}
```

#### log_rotation

**Description**: Log file rotation settings (spdlog only)

**Format**: `"size:max_files"` (size in MB)

**Example**:
```json
{
    "log_rotation": "10:5"
}
```

This rotates the log file when it reaches 10 MB, keeping up to 5 backup files.

### Threading Configuration

#### max_threads

**Description**: Maximum number of worker threads

**Values**: 1 - CPU core count

**Default**: `4`

**Example**:
```json
{
    "max_threads": "8"
}
```

**Performance Tuning**:
```cpp
int cores = std::thread::hardware_concurrency();
// Set max_threads = cores for CPU-bound tasks
// Set max_threads = cores * 2 for I/O-bound tasks
```

#### thread_stack_size

**Description**: Stack size per thread (in KB)

**Default**: System default

**Example**:
```json
{
    "thread_stack_size": "8192"
}
```

### Task Configuration

#### task_timeout

**Description**: Task execution timeout (milliseconds)

**Default**: `5000` (5 seconds)

**Example**:
```json
{
    "task_timeout": "10000"
}
```

#### task_priority

**Description**: Task execution priority

**Values**:
- `low` - Lower priority, runs when system idle
- `normal` - Default priority
- `high` - Higher priority, preempts normal tasks
- `realtime` - Real-time priority (requires elevated privileges)

**Default**: `normal`

**Example**:
```json
{
    "task_priority": "high"
}
```

---

## Advanced Settings

### MQTT Configuration

#### mqtt_broker

**Description**: MQTT broker connection URL

**Format**: `protocol://host:port`

**Protocols**:
- `mqtt://` - Unencrypted (TCP)
- `mqtts://` - Encrypted (TLS/SSL)
- `ws://` - WebSocket
- `wss://` - Secure WebSocket

**Example**:
```json
{
    "mqtt_broker": "mqtts://broker.example.com:8883"
}
```

#### mqtt_client_id

**Description**: MQTT client identifier

**Default**: `virtualbus_<timestamp>`

**Example**:
```json
{
    "mqtt_client_id": "my_device_001"
}
```

#### mqtt_username / mqtt_password

**Description**: MQTT authentication credentials

**Example**:
```json
{
    "mqtt_username": "iotuser",
    "mqtt_password": "secure_password"
}
```

**Security Note**: 
- Store in separate environment file
- Never commit credentials to repository
- Use proper file permissions (600)

#### mqtt_topics

**Description**: MQTT publish/subscribe topics

**Example**:
```json
{
    "mqtt_topics": {
        "status": "devices/device1/status",
        "commands": "devices/device1/commands",
        "telemetry": "devices/device1/telemetry"
    }
}
```

#### mqtt_qos

**Description**: MQTT Quality of Service level

**Values**:
- `0` - At most once (fire and forget)
- `1` - At least once (acknowledged)
- `2` - Exactly once (guaranteed once)

**Default**: `1`

**Example**:
```json
{
    "mqtt_qos": "1"
}
```

#### mqtt_keepalive

**Description**: MQTT keep-alive interval (seconds)

**Default**: `60`

**Example**:
```json
{
    "mqtt_keepalive": "30"
}
```

### Watchdog Configuration

#### enable_watchdog

**Description**: Enable system watchdog timer

**Default**: `false`

**Example**:
```json
{
    "enable_watchdog": true
}
```

#### watchdog_timeout

**Description**: Watchdog timeout (milliseconds)

**Default**: `30000` (30 seconds)

**Example**:
```json
{
    "watchdog_timeout": "60000"
}
```

### Database Configuration (Optional)

#### database_host

**Description**: Database server hostname

**Example**:
```json
{
    "database_host": "db.example.com"
}
```

#### database_port

**Description**: Database server port

**Default**: `5432` (PostgreSQL)

**Example**:
```json
{
    "database_port": "5432"
}
```

#### database_name

**Description**: Database name

**Example**:
```json
{
    "database_name": "virtualbus_db"
}
```

#### database_user / database_password

**Description**: Database credentials

**Example**:
```json
{
    "database_user": "dbuser",
    "database_password": "dbpassword"
}
```

### Cache Configuration (Optional)

#### cache_enabled

**Description**: Enable caching

**Default**: `false`

**Example**:
```json
{
    "cache_enabled": true
}
```

#### cache_ttl

**Description**: Cache time-to-live (seconds)

**Default**: `3600` (1 hour)

**Example**:
```json
{
    "cache_ttl": "7200"
}
```

#### cache_max_size

**Description**: Maximum cache size (entries)

**Default**: `1000`

**Example**:
```json
{
    "cache_max_size": "5000"
}
```

---

## Environment Variables

### Using Environment Variables

VirtualBus can read configuration from environment variables:

```bash
export VIRTUALBUS_LOG_LEVEL=debug
export VIRTUALBUS_MAX_THREADS=8
export VIRTUALBUS_MQTT_BROKER=mqtts://broker.example.com:8883
```

**Naming Convention**: `VIRTUALBUS_<SETTING_NAME>`

### Environment Variable Override

Environment variables override configuration file settings:

```
Default < config.json < Environment Variables < Command Line
```

### Common Environment Variables

```bash
# Logging
VIRTUALBUS_LOG_LEVEL=debug
VIRTUALBUS_LOG_FILE=logs/app.log

# Threading
VIRTUALBUS_MAX_THREADS=8

# MQTT
VIRTUALBUS_MQTT_BROKER=mqtt://localhost:1883
VIRTUALBUS_MQTT_CLIENT_ID=device_001
VIRTUALBUS_MQTT_USERNAME=user
VIRTUALBUS_MQTT_PASSWORD=pass

# Database
VIRTUALBUS_DATABASE_HOST=localhost
VIRTUALBUS_DATABASE_PORT=5432
VIRTUALBUS_DATABASE_NAME=virtualbus

# Watchdog
VIRTUALBUS_ENABLE_WATCHDOG=true
VIRTUALBUS_WATCHDOG_TIMEOUT=30000
```

---

## Configuration Examples

### Example 1: Development Configuration

**config.json**:
```json
{
    "log_level": "debug",
    "log_file": "logs/dev.log",
    "max_threads": "2",
    "mqtt_broker": "mqtt://localhost:1883",
    "mqtt_client_id": "dev_client",
    "task_timeout": "10000",
    "enable_watchdog": false,
    "database_host": "localhost",
    "database_port": "5432",
    "cache_enabled": false
}
```

### Example 2: Production Configuration

**config.json**:
```json
{
    "log_level": "warn",
    "log_file": "/var/log/virtualbus/app.log",
    "log_rotation": "50:10",
    "max_threads": "16",
    "mqtt_broker": "mqtts://broker.production.com:8883",
    "mqtt_client_id": "prod_device_001",
    "mqtt_username": "prod_user",
    "mqtt_password": "secure_prod_password",
    "task_timeout": "5000",
    "task_priority": "high",
    "enable_watchdog": true,
    "watchdog_timeout": "60000",
    "database_host": "db.production.com",
    "database_port": "5432",
    "database_name": "virtualbus_prod",
    "cache_enabled": true,
    "cache_ttl": "7200",
    "cache_max_size": "10000"
}
```

### Example 3: Embedded Systems Configuration

**config.json**:
```json
{
    "log_level": "info",
    "log_file": "/tmp/virtualbus.log",
    "max_threads": "2",
    "mqtt_broker": "mqtt://edge_gateway:1883",
    "mqtt_client_id": "embedded_device_123",
    "mqtt_qos": "0",
    "task_timeout": "2000",
    "task_priority": "realtime",
    "enable_watchdog": true,
    "watchdog_timeout": "20000",
    "cache_enabled": true,
    "cache_ttl": "1800",
    "cache_max_size": "500"
}
```

### Example 4: IoT Configuration

**config.json**:
```json
{
    "log_level": "info",
    "max_threads": "4",
    "mqtt_broker": "mqtts://iot.azure.com:8883",
    "mqtt_client_id": "iot_device_456",
    "mqtt_topics": {
        "status": "devices/device456/status",
        "commands": "devices/device456/commands",
        "telemetry": "devices/device456/telemetry"
    },
    "mqtt_keepalive": "30",
    "task_timeout": "5000",
    "enable_watchdog": true,
    "watchdog_timeout": "45000",
    "cache_enabled": true,
    "cache_ttl": "3600"
}
```

---

## Configuration API

### Loading Configuration

```cpp
#include "Configuration.h"
#include "JsonStorage.h"

// Get singleton instance
Configuration& config = Configuration::getInstance();

// Set logger
auto logger = std::make_shared<SpdLogWrapper>();
config.setLogger(logger);

// Set storage backend
auto storage = std::make_unique<JsonStorage>(logger);
config.setStorage(std::move(storage));

// Load configuration file
if (!config.load("config.json")) {
    logger->error("Failed to load configuration");
    return -1;
}
```

### Reading Configuration Values

```cpp
// Get configuration value (returns string)
std::string logLevel = config.getConfig("log_level");
std::string maxThreads = config.getConfig("max_threads");

// Convert to appropriate type
int numThreads = std::stoi(maxThreads);
bool watchdogEnabled = config.getConfig("enable_watchdog") == "true";
```

### Writing Configuration Values

```cpp
// Set configuration value
config.setConfig("log_level", "debug");
config.setConfig("max_threads", "8");
config.setConfig("enable_watchdog", "true");

// Save to file
if (!config.save("updated_config.json")) {
    logger->error("Failed to save configuration");
}
```

### Checking Configuration Keys

```cpp
// Check if key exists
if (config.hasConfig("mqtt_broker")) {
    std::string broker = config.getConfig("mqtt_broker");
}

// Get all configuration
auto allConfig = config.getAll();
for (const auto& [key, value] : allConfig) {
    std::cout << key << " = " << value << std::endl;
}
```

### Default Values

```cpp
// Use getConfigOr() for default values
std::string level = config.getConfig("log_level", "info");
int threads = std::stoi(config.getConfig("max_threads", "4"));
```

---

## Best Practices

### 1. Configuration Structure

**Do**:
- Keep configuration simple and readable
- Use logical grouping of related settings
- Document all settings with comments

**Don't**:
- Store secrets in configuration files
- Use deeply nested structures
- Mix configuration with application logic

### 2. Security

**Do**:
- Store credentials in environment variables
- Use file permissions (chmod 600)
- Use HTTPS/TLS for remote configuration
- Validate configuration values

**Don't**:
- Store passwords in version control
- Use default credentials in production
- Trust external configuration without validation
- Log sensitive configuration values

### 3. Defaults

**Do**:
- Provide sensible defaults for all settings
- Use conservative defaults (less is faster)
- Document default values

**Don't**:
- Require all settings to be configured
- Use overly aggressive defaults
- Change defaults frequently

### 4. Validation

```cpp
// Validate configuration after loading
bool validateConfig(Configuration& config, std::shared_ptr<ILogger> logger) {
    // Check required settings
    if (config.getConfig("log_level").empty()) {
        logger->error("log_level is required");
        return false;
    }
    
    // Validate numeric values
    int threads = std::stoi(config.getConfig("max_threads"));
    if (threads < 1 || threads > 128) {
        logger->error("max_threads must be between 1 and 128");
        return false;
    }
    
    return true;
}
```

### 5. Hot Reload (Advanced)

```cpp
// Reload configuration at runtime
bool reloadConfiguration(Configuration& config, 
                        const std::string& filename,
                        std::shared_ptr<ILogger> logger) {
    if (!config.load(filename)) {
        logger->error("Failed to reload configuration");
        return false;
    }
    
    logger->info("Configuration reloaded successfully");
    return true;
}
```

---

## Troubleshooting

### Issue: Configuration File Not Found

**Error Message**: `Failed to load configuration file`

**Solutions**:
1. Check file path is correct
2. Verify file exists: `ls -l config.json`
3. Check file permissions: `chmod 644 config.json`
4. Use absolute path: `/full/path/to/config.json`

```cpp
// Use absolute path
if (!config.load("/home/user/VirtualBus/config.json")) {
    logger->error("Failed to load config");
}
```

### Issue: Invalid Configuration Format

**Error Message**: `Failed to parse configuration`

**Solutions**:
1. Validate JSON: `python3 -m json.tool config.json`
2. Check for syntax errors (missing commas, quotes)
3. Use UTF-8 encoding without BOM
4. Avoid circular references

### Issue: Missing Required Settings

**Error Message**: `Configuration value not found: <key>`

**Solutions**:
1. Add missing setting to config.json
2. Use getConfig with default value:
   ```cpp
   std::string value = config.getConfig("key", "default");
   ```
3. Provide environment variable: `export VIRTUALBUS_KEY=value`

### Issue: Type Conversion Errors

**Error Message**: `invalid_argument in stoi()`

**Solutions**:
1. Verify configuration value is correct type
2. Check for empty or null values
3. Use try-catch for conversion:
   ```cpp
   try {
       int val = std::stoi(config.getConfig("max_threads"));
   } catch (const std::invalid_argument& e) {
       logger->error("Invalid number format");
   }
   ```

### Issue: MQTT Connection Fails

**Error Message**: `Failed to connect to MQTT broker`

**Solutions**:
1. Check broker address: `ping broker.example.com`
2. Verify port is correct: `nc -zv broker.example.com 1883`
3. Check firewall: `sudo ufw allow 1883`
4. Verify credentials in configuration
5. Check broker is running: `mosquitto -v`

**Example**:
```json
{
    "mqtt_broker": "mqtt://broker.local:1883",
    "mqtt_username": "test",
    "mqtt_password": "test"
}
```

### Issue: Performance Problems

**Possible Causes**:
- Too many threads (`max_threads` too high)
- Excessive logging (`log_level` set to debug)
- Large cache size with short TTL

**Solutions**:
```json
{
    "log_level": "warn",
    "max_threads": 4,
    "cache_ttl": "7200"
}
```

---

## Configuration Checklist

- [ ] Load configuration early in main()
- [ ] Set logger before loading config
- [ ] Validate all configuration values
- [ ] Handle missing configuration gracefully
- [ ] Use environment variables for secrets
- [ ] Keep configuration in version control (without secrets)
- [ ] Document all configuration options
- [ ] Test configuration on target platform
- [ ] Monitor configuration changes in logs
- [ ] Backup configuration before updates

---

**Configuration documentation complete!** See [USAGE.md](./USAGE.md) for usage examples.
