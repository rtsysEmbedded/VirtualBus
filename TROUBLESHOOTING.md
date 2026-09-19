# VirtualBus Troubleshooting Guide

Common issues and solutions for VirtualBus development and deployment.

## Table of Contents

1. [Build Issues](#build-issues)
2. [Runtime Issues](#runtime-issues)
3. [Configuration Issues](#configuration-issues)
4. [Performance Issues](#performance-issues)
5. [Logging and Debugging](#logging-and-debugging)
6. [Platform-Specific Issues](#platform-specific-issues)
7. [FAQ](#faq)
8. [Getting Help](#getting-help)

---

## Build Issues

### CMake Not Found

**Error**: `cmake: command not found` or `CMake not found`

**Solutions**:

1. **Install CMake**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake
   
   # macOS
   brew install cmake
   
   # Windows (chocolatey)
   choco install cmake
   ```

2. **Check CMake Version**:
   ```bash
   cmake --version  # Should be 3.15 or higher
   ```

3. **Add to PATH**:
   ```bash
   # Linux/macOS: Usually automatic after install
   
   # Windows: Add CMake bin directory to PATH
   # C:\Program Files\CMake\bin
   ```

### C++ Compiler Not Found

**Error**: `No C++ compiler found` or `c++ compiler not found`

**Solutions**:

1. **Install Build Tools**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential
   
   # Fedora
   sudo dnf install gcc-c++ make
   
   # macOS
   xcode-select --install
   ```

2. **Verify Compiler**:
   ```bash
   g++ --version  # For GCC
   clang++ --version  # For Clang
   ```

3. **Specify Compiler**:
   ```bash
   cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/g++
   ```

### OpenSSL Not Found

**Error**: `OpenSSL not found` or `libssl-dev not found`

**Solutions**:

1. **Install OpenSSL**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libssl-dev
   
   # Fedora
   sudo dnf install openssl-devel
   
   # macOS
   brew install openssl
   ```

2. **Set OpenSSL Path**:
   ```bash
   # macOS (if installed with brew)
   cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl
   
   # Linux (if in non-standard location)
   cmake .. -DOPENSSL_ROOT_DIR=/custom/path
   ```

3. **Check OpenSSL Installation**:
   ```bash
   openssl version
   pkg-config --modversion openssl
   ```

### Missing Dependencies

**Error**: `error: 'xxx.h' file not found`

**Solutions**:

1. **Run Fetch and Build Script**:
   ```bash
   bash fetch_and_build.sh  # Linux/macOS
   ```

2. **Manual Dependency Installation**:
   ```bash
   # Install all development headers
   sudo apt-get install libssl-dev libcurl4-openssl-dev
   ```

3. **Check Include Directories**:
   ```bash
   cmake .. -DCMAKE_MESSAGE_LOG_LEVEL=VERBOSE
   ```

### Build Fails with "Permission Denied"

**Error**: `Permission denied` during build

**Solutions**:

1. **Check Directory Permissions**:
   ```bash
   ls -la .
   chmod u+x build
   ```

2. **Use Sudo Carefully**:
   ```bash
   # Better: Fix permissions instead
   # NOT recommended: sudo cmake ..
   ```

3. **Rebuild Directory**:
   ```bash
   rm -rf build
   mkdir build && cd build
   cmake ..
   make
   ```

### Linker Errors

**Error**: `undefined reference to 'xxx'` or `symbol lookup error`

**Solutions**:

1. **Check Linked Libraries**:
   ```bash
   # View linked libraries
   ldd ./CPPProject  # Linux
   otool -L ./CPPProject  # macOS
   ```

2. **Rebuild with Verbose Output**:
   ```bash
   cmake --build . --verbose
   # Or
   make VERBOSE=1
   ```

3. **Check Library Paths**:
   ```bash
   cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
   ```

---

## Runtime Issues

### Application Crashes on Startup

**Error**: `Segmentation fault` or `Access violation`

**Solutions**:

1. **Run with Debugger**:
   ```bash
   gdb ./CPPProject
   (gdb) run
   # Note: Shows exact line causing crash
   ```

2. **Enable Logging**:
   ```json
   {
       "log_level": "debug"
   }
   ```

3. **Check Configuration File**:
   ```bash
   # Verify config.json exists
   ls -la config.json
   
   # Validate JSON syntax
   python3 -m json.tool config.json
   ```

### Message Delivery Failure

**Issue**: Messages sent but not received

**Solutions**:

1. **Verify Tasks Are Attached**:
   ```cpp
   ReturnType result = bus.attach(taskId, taskName);
   if (result != ReturnType::OK) {
       logger->error("Failed to attach task");
   }
   ```

2. **Check Task IDs**:
   ```cpp
   // Ensure sender and receiver use different IDs
   bus.sendMessage(senderId, msg);  // senderId
   bus.receiveMessage(receiverId, msg);  // receiverId (different)
   ```

3. **Verify Bus is Running**:
   ```cpp
   // Bus must be operational
   // Don't call shutdown() before send/receive
   ```

### Task Hangs or Deadlock

**Issue**: Application freezes or becomes unresponsive

**Solutions**:

1. **Check for Mutex Deadlock**:
   ```cpp
   // Don't acquire same mutex twice
   // Example of WRONG code:
   {
       std::lock_guard<std::mutex> lock(mutex_);
       // Don't call functions that also lock mutex_
   }
   ```

2. **Use Timeout**:
   ```cpp
   // Add timeout to prevent infinite waits
   std::chrono::seconds timeout(5);
   ```

3. **Monitor Thread Status**:
   ```bash
   # View running threads
   ps -eLf | grep CPPProject
   
   # View thread stack traces
   gdb -p <PID>
   (gdb) info threads
   (gdb) bt all
   ```

4. **Check for Circular Dependencies**:
   ```
   Task A sends to Task B
   Task B sends to Task A  <- Potential deadlock
   ```

### Memory Leaks

**Issue**: Memory usage grows over time

**Solutions**:

1. **Use Valgrind** (Linux):
   ```bash
   # Check for memory leaks
   valgrind --leak-check=full --show-leak-kinds=all ./CPPProject
   ```

2. **Use Address Sanitizer**:
   ```bash
   # Compile with sanitizer
   cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address"
   make
   ./CPPProject
   ```

3. **Check for Resource Leaks**:
   ```cpp
   // Ensure proper cleanup
   task.stop();
   task.join();  // Don't skip this
   bus.shutdown();
   ```

### Thread Synchronization Issues

**Issue**: Race conditions or data corruption

**Solutions**:

1. **Use Thread Sanitizer**:
   ```bash
   cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread"
   ```

2. **Lock All Shared Access**:
   ```cpp
   // Use lock_guard for all shared data access
   std::lock_guard<std::mutex> lock(mutex_);
   // Access shared data
   ```

3. **Check Message Passing**:
   ```cpp
   // Ensure messages are properly synchronized
   bus.sendMessage(senderId, msg);
   ```

---

## Configuration Issues

### Configuration File Not Found

**Error**: `Failed to load configuration file`

**Solutions**:

1. **Verify File Exists**:
   ```bash
   ls -la config.json
   pwd  # Check current directory
   ```

2. **Use Absolute Path**:
   ```cpp
   if (!config.load("/full/path/to/config.json")) {
       logger->error("Failed to load config");
   }
   ```

3. **Check File Permissions**:
   ```bash
   chmod 644 config.json  # Readable by all
   chmod 600 config.json  # Readable by owner only (for secrets)
   ```

### Invalid Configuration Format

**Error**: `Failed to parse configuration` or `JSON parse error`

**Solutions**:

1. **Validate JSON**:
   ```bash
   python3 -m json.tool config.json
   # or
   jq . config.json
   ```

2. **Fix Common Errors**:
   - Missing commas between properties
   - Trailing commas (not allowed in JSON)
   - Unquoted keys or values
   - Single quotes instead of double quotes

3. **Example Valid Config**:
   ```json
   {
       "log_level": "info",
       "max_threads": "4"
   }
   ```

### Configuration Value Not Found

**Error**: `Configuration value not found: key`

**Solutions**:

1. **Check Key Name**:
   ```json
   {
       "log_level": "info"  // Exact key name matters
   }
   ```

2. **Use Default Value**:
   ```cpp
   std::string value = config.getConfig("missing_key", "default_value");
   ```

3. **Verify Configuration Loaded**:
   ```cpp
   if (config.load("config.json")) {
       std::string value = config.getConfig("log_level");
   }
   ```

### Type Conversion Error

**Error**: `invalid_argument in stoi()` or similar

**Solutions**:

1. **Validate Type Before Conversion**:
   ```cpp
   std::string maxThreadsStr = config.getConfig("max_threads");
   try {
       int maxThreads = std::stoi(maxThreadsStr);
   } catch (const std::invalid_argument& e) {
       logger->error("Invalid number format: " + maxThreadsStr);
   }
   ```

2. **Check Configuration Values**:
   ```bash
   grep max_threads config.json  # Should be numeric string
   ```

---

## Performance Issues

### High CPU Usage

**Causes and Solutions**:

1. **Busy Polling Loop**:
   ```cpp
   // BAD: Consumes CPU
   while (running_) {
       std::shared_ptr<VirtualBusCmd> msg;
       bus.receiveMessage(id_, msg);  // No sleep!
   }
   
   // GOOD: Uses less CPU
   while (running_) {
       std::shared_ptr<VirtualBusCmd> msg;
       if (bus.receiveMessage(id_, msg)) {
           processMessage(msg);
       }
       std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }
   ```

2. **Use Callbacks Instead of Polling**:
   ```cpp
   // More efficient
   bus.registerCallback(id_, [](auto msg) {
       processMessage(msg);
   });
   ```

3. **Reduce Logging Level**:
   ```json
   {
       "log_level": "warn"  // Less logging = less CPU
   }
   ```

### High Memory Usage

**Causes and Solutions**:

1. **Message Queue Overflow**:
   ```cpp
   // Limit queue size
   // Send fewer messages or process faster
   ```

2. **Memory Leak in Callbacks**:
   ```cpp
   // Ensure callbacks don't hold references
   bus.registerCallback(id_, [this](auto msg) {
       // 'this' might cause issues if not careful
       processMessage(msg);
   });
   ```

3. **Cache Configuration**:
   ```json
   {
       "cache_enabled": false,
       "cache_max_size": "1000"  // Limit cache size
   }
   ```

### Slow Message Processing

**Solutions**:

1. **Use Threading**:
   ```cpp
   // Process messages in parallel
   ThreadPool pool(4);
   pool.submit([msg]() { processMessage(msg); });
   ```

2. **Batch Processing**:
   ```cpp
   std::vector<std::shared_ptr<VirtualBusCmd>> batch;
   // Collect messages into batch
   // Process batch efficiently
   ```

3. **Profile Code**:
   ```bash
   # Using perf (Linux)
   perf record ./CPPProject
   perf report
   ```

---

## Logging and Debugging

### No Log Output

**Issue**: Logs not appearing

**Solutions**:

1. **Check Log Level**:
   ```json
   {
       "log_level": "debug"  // Lower level to see more logs
   }
   ```

2. **Verify Logger is Configured**:
   ```cpp
   auto logger = std::make_shared<SpdLogWrapper>();
   bus(logger);  // Pass logger to bus
   ```

3. **Check Log Output**:
   ```bash
   # For file logging
   tail -f logs/virtualbus.log
   ```

### Log File Not Created

**Solutions**:

1. **Check Directory Permissions**:
   ```bash
   ls -la logs/
   chmod 755 logs
   ```

2. **Verify Log File Path**:
   ```json
   {
       "log_file": "./logs/virtualbus.log"  // Relative path
   }
   ```

3. **Use Absolute Path**:
   ```json
   {
       "log_file": "/var/log/virtualbus/app.log"
   }
   ```

### Switching Between Loggers

**Choose at Compile Time**:
```bash
# Build with spdlog
cmake .. -DENABLE_SPDLOG=ON

# Build with stdout logger
cmake .. -DENABLE_SPDLOG=OFF
```

---

## Platform-Specific Issues

### Linux

**Issue**: `libpaho-mqtt3a.so: cannot open shared object file`

**Solution**:
```bash
# Install package or set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/lib:$LD_LIBRARY_PATH
./CPPProject
```

### macOS

**Issue**: `dyld: Library not loaded`

**Solution**:
```bash
# Check library path
otool -L ./CPPProject

# Set library path
export DYLD_LIBRARY_PATH=/usr/local/opt/openssl/lib:$DYLD_LIBRARY_PATH
./CPPProject
```

### Windows

**Issue**: `The system cannot find the file specified`

**Solution**:
```powershell
# Add dependencies to PATH
$env:PATH += ";C:\path\to\libraries\bin"
.\CPPProject.exe
```

---

## FAQ

### Q: How do I increase logging detail?

**A**: Change `log_level` in config.json to `debug`:
```json
{
    "log_level": "debug"
}
```

### Q: How do I reduce CPU usage?

**A**:
1. Use callbacks instead of polling
2. Increase sleep duration in polling loops
3. Reduce logging level
4. Adjust thread count

### Q: How do I connect to a remote MQTT broker?

**A**:
```json
{
    "mqtt_broker": "mqtts://broker.example.com:8883",
    "mqtt_username": "user",
    "mqtt_password": "pass"
}
```

### Q: How do I debug hanging tasks?

**A**:
```bash
# Attach debugger
gdb -p <PID>
(gdb) info threads
(gdb) bt  # Backtrace for each thread
```

### Q: How do I monitor resource usage?

**A**:
```bash
# Real-time monitoring
top -p <PID>

# Memory usage
ps aux | grep CPPProject
```

### Q: Can I use VirtualBus without configuration file?

**A**: Yes, use defaults:
```cpp
Configuration& config = Configuration::getInstance();
// Use defaults without loading file
std::string level = config.getConfig("log_level", "info");
```

### Q: How do I handle task failures gracefully?

**A**:
```cpp
try {
    // Task work
} catch (const std::exception& e) {
    logger->error("Task failed: " + std::string(e.what()));
    // Attempt recovery or shutdown
}
```

---

## Getting Help

### When Reporting Issues

Include:
1. **VirtualBus Version**: Check version.txt
2. **Platform**: OS, architecture, compiler version
3. **Build Configuration**: CMake flags used
4. **Configuration File**: Relevant portions (without secrets)
5. **Logs**: Full log output showing the issue
6. **Reproduction Steps**: How to reproduce the issue
7. **Expected vs Actual**: What should happen vs what does

### Resources

- **GitHub Issues**: https://github.com/rtsysembedded/VirtualBus/issues
- **Documentation**: See README.md and other .md files
- **Code Examples**: Check EXAMPLES.md
- **Configuration Guide**: See CONFIGURATION.md

### Debug Mode

**Enable Full Debugging**:
```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Run with sanitizers
export ASAN_OPTIONS=verbosity=1:halt_on_error=1
./CPPProject
```

---

## Checklist for Troubleshooting

- [ ] Check CMake version (3.15+)
- [ ] Verify C++ compiler (C++17 capable)
- [ ] Confirm OpenSSL installation
- [ ] Validate configuration file (JSON syntax)
- [ ] Check file permissions
- [ ] Review log output
- [ ] Run with verbose output
- [ ] Try minimal example
- [ ] Check system resources (disk, memory, CPU)
- [ ] Update dependencies
- [ ] Clean and rebuild
- [ ] Test on different platform if possible

---

**Still need help?** See [README.md](./README.md) or create an issue on GitHub.
