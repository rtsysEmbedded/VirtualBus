# C++ Project

This is a reorganized C++ project structure with the following layout:

- `src/` - Source files.
- `include/` - Header files.
- `libs/` - External libraries.
- `tests/` - Unit tests.
- `cmake/` - CMake modules.
- `build/` - Build output (ignored in version control).


- Prerequisites for Each Platform
Linux: Install OpenSSL via your package manager:
bash
Copy code
sudo apt-get install libssl-dev
macOS: Install OpenSSL using Homebrew:
bash
Copy code
brew install openssl
Windows:
Install OpenSSL using a package like vcpkg.
Example:
bash
Copy code
vcpkg install openssl:x64-windows
Add vcpkg to your CMake toolchain:
bash
Copy code
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

## Comparison with Reactive Programming Models

This project implements a Virtual Bus system using a traditional publish-subscribe pattern, which can be compared with reactive programming models like RxCpp:

### Architecture Comparison

#### VirtualBus (Current Implementation):
- Traditional publish-subscribe pattern
- Explicit SendTask and ReceiveTask components
- Thread-safe communication
- JSON-based configuration
- AUTOSAR Adaptive compliant
- Better suited for automotive/embedded systems

#### Reactive Models (e.g., RxCpp):
- Observable/Observer pattern
- Push-based data streams
- Supports operation chaining (LINQ-style)
- Built-in scheduling and concurrency
- More functional programming approach
- Better suited for event-driven applications

### Key Differences

1. **Data Flow**:
   - VirtualBus: Message-based with explicit send/receive tasks
   - Reactive: Stream-based with push notifications and transformations

2. **Flexibility**:
   - VirtualBus: More structured, optimized for automotive/embedded systems
   - Reactive: More flexible, better for general event-driven applications

3. **Error Handling**:
   - VirtualBus: Explicit error handling system
   - Reactive: Built-in error propagation through streams

### Future Enhancement Possibilities
- Integration of reactive patterns for event handling
- Implementation of operator chaining for message transformation
- Addition of stream-based APIs alongside current message-based ones