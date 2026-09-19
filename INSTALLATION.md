# VirtualBus Installation Guide

This guide provides detailed instructions for installing VirtualBus on various platforms.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Linux Installation](#linux-installation)
3. [macOS Installation](#macos-installation)
4. [Windows Installation](#windows-installation)
5. [Build Configuration](#build-configuration)
6. [Verification](#verification)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Software

- **C++ Compiler**: GCC 7.0+ (Linux), Clang 5.0+ (macOS/Linux), MSVC 2017+ (Windows)
- **CMake**: Version 3.15 or higher
- **Git**: For cloning the repository
- **Python**: Python 3.6+ (optional, for build scripts)
- **OpenSSL**: For MQTT secure connections

### Development Tools

- **Build Tools**:
  - Linux: `build-essential`
  - macOS: Xcode Command Line Tools
  - Windows: Visual Studio 2017+ or MinGW

---

## Linux Installation

### Ubuntu/Debian

#### 1. Install Dependencies

```bash
# Update package manager
sudo apt-get update

# Install build tools and required libraries
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    python3 \
    python3-pip
```

#### 2. Clone the Repository

```bash
git clone https://github.com/rtsysembedded/VirtualBus.git
cd VirtualBus
```

#### 3. Create Build Directory

```bash
mkdir build
cd build
```

#### 4. Configure with CMake

```bash
# Default configuration
cmake ..

# Or with specific options
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_SPDLOG=ON
```

#### 5. Build the Project

```bash
# Using cmake
cmake --build . --config Release

# Or using make
make -j$(nproc)
```

#### 6. Verify Installation

```bash
# Check if executable was created
ls -la CPPProject

# Run the application
./CPPProject
```

### Fedora/RHEL/CentOS

```bash
# Install dependencies
sudo dnf install -y \
    cmake \
    gcc-c++ \
    make \
    openssl-devel \
    git \
    python3

# Clone and build
git clone https://github.com/rtsysembedded/VirtualBus.git
cd VirtualBus
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S base-devel cmake openssl git python

# Clone and build
git clone https://github.com/rtsysembedded/VirtualBus.git
cd VirtualBus
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## macOS Installation

### Using Homebrew (Recommended)

#### 1. Install Homebrew (if not installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. Install Dependencies

```bash
# Install required packages
brew install cmake openssl git python@3.11

# Add OpenSSL to path (if needed)
export LDFLAGS="-L/usr/local/opt/openssl/lib"
export CPPFLAGS="-I/usr/local/opt/openssl/include"
export PKG_CONFIG_PATH="/usr/local/opt/openssl/lib/pkgconfig"
```

#### 3. Clone and Build

```bash
git clone https://github.com/rtsysembedded/VirtualBus.git
cd VirtualBus
mkdir build && cd build

# Configure with OpenSSL path
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
    -DENABLE_SPDLOG=ON

# Build
make -j$(sysctl -n hw.ncpu)
```

#### 4. Run

```bash
./CPPProject
```

### Using Xcode

#### 1. Install Xcode Command Line Tools

```bash
xcode-select --install
```

#### 2. Install Dependencies

```bash
brew install cmake openssl
```

#### 3. Generate Xcode Project

```bash
mkdir build && cd build
cmake .. -G Xcode -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl
```

#### 4. Build with Xcode

```bash
cmake --build . --config Release
```

---

## Windows Installation

### Using Visual Studio 2019/2022

#### 1. Install Visual Studio

- Download Visual Studio Community (free)
- Install with "Desktop development with C++"
- Include CMake tools

#### 2. Install OpenSSL

Option A: Using vcpkg (Recommended)

```powershell
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Install OpenSSL
.\vcpkg install openssl:x64-windows

# Integrate vcpkg (optional but recommended)
.\vcpkg integrate install
```

Option B: Pre-built OpenSSL

- Download from: https://slproweb.com/products/Win32OpenSSL.html
- Install to `C:\OpenSSL` or similar

#### 3. Clone Repository

```powershell
git clone https://github.com/rtsysembedded/VirtualBus.git
cd VirtualBus
```

#### 4. Configure with CMake (Visual Studio)

```powershell
# With vcpkg
mkdir build
cd build
cmake .. `
    -G "Visual Studio 16 2019" `
    -DCMAKE_TOOLCHAIN_FILE="<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake" `
    -DCMAKE_BUILD_TYPE=Release

# Or with pre-built OpenSSL
cmake .. `
    -G "Visual Studio 16 2019" `
    -DOPENSSL_ROOT_DIR="C:\OpenSSL" `
    -DCMAKE_BUILD_TYPE=Release
```

#### 5. Build

```powershell
# Using Visual Studio
cmake --build . --config Release

# Or open the generated Visual Studio solution
# Open .\CPPProject.sln in Visual Studio and build from IDE
```

### Using MinGW

```powershell
# Install MinGW (e.g., via MSYS2)
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-openssl

# Build
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### Using PowerShell Script

```powershell
# Run the provided build script
.\fetch_and_build.ps1
```

---

## Build Configuration

### CMake Options

#### Enable/Disable spdlog

```bash
# Enable spdlog (default: ON)
cmake .. -DENABLE_SPDLOG=ON

# Disable spdlog (use stdout logging)
cmake .. -DENABLE_SPDLOG=OFF
```

#### Build Type

```bash
# Release (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Debug (with symbols)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# RelWithDebInfo (release with debug info)
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# MinSizeRel (minimum size)
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
```

#### Custom Toolchain

```bash
# Use a specific compiler
cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/g++-11

# Use custom toolchain file
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake
```

### Advanced Configuration

```bash
# Install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/virtualbus

# Parallel build
cmake --build . -j 8

# Verbose output
cmake --build . --verbose
```

---

## Build Scripts

### Using Bash Script (Linux/macOS)

```bash
# Run the provided build script
bash fetch_and_build.sh

# Script features:
# - Fetches external libraries
# - Configures CMake
# - Builds the project
# - Runs tests (if available)
```

### Using PowerShell Script (Windows)

```powershell
# Run the provided build script
.\fetch_and_build.ps1

# Script features:
# - Fetches dependencies
# - Configures Visual Studio
# - Builds the project
```

### Using Python Script (Cross-platform)

```bash
# Run the cross-platform build script
python3 fetch_and_build.py

# Supports Linux, macOS, and Windows
# Automatically detects the platform
```

---

## Verification

### Test the Installation

#### 1. Check CMake Configuration

```bash
# In the build directory
cmake .. --debug-output 2>&1 | grep -i "openssl\|spdlog"
```

#### 2. Build Verification

```bash
# Check if executable was created
ls -la ./CPPProject  # Linux/macOS
dir .\CPPProject.exe # Windows
```

#### 3. Run the Application

```bash
# Linux/macOS
./CPPProject

# Windows
.\CPPProject.exe
```

#### 4. Expected Output

```
###############################################
Project Version: 1.0.0
###############################################
[timestamp] [info] spdlog set for logging
[timestamp] [info] Log Level: info
[timestamp] [info] Max Threads: 4
[timestamp] [info] SendTask: Sent InverterCommand...
[timestamp] [info] ReceiveTask: Message received...
```

### Verify Libraries

```bash
# Check linked libraries (Linux)
ldd ./CPPProject

# Check linked libraries (macOS)
otool -L ./CPPProject

# Should show:
# - libpaho-mqtt3a
# - libpaho-mqttpp3
# - libspdlog
# - libssl, libcrypto (OpenSSL)
# - libpthread
```

---

## Installation Verification Checklist

- [ ] CMake is installed and working
- [ ] C++ compiler is available
- [ ] OpenSSL is installed and found by CMake
- [ ] Git repository cloned successfully
- [ ] `mkdir build && cd build` executed
- [ ] `cmake ..` completed without errors
- [ ] `cmake --build .` completed successfully
- [ ] Executable `CPPProject` created in build directory
- [ ] Application runs without errors
- [ ] Configuration file `config.json` exists
- [ ] Logging output is visible

---

## Troubleshooting Installation

### CMake Configuration Issues

**Error: "CMake not found"**
```bash
# Install CMake
sudo apt-get install cmake          # Ubuntu/Debian
brew install cmake                  # macOS
choco install cmake                 # Windows (with chocolatey)
```

**Error: "C++ compiler not found"**
```bash
# Linux
sudo apt-get install build-essential g++

# macOS
xcode-select --install

# Windows
# Install Visual Studio with C++ tools
```

**Error: "OpenSSL not found"**
```bash
# Linux
sudo apt-get install libssl-dev

# macOS
brew install openssl

# Windows
# Use vcpkg: vcpkg install openssl:x64-windows
```

### Build Issues

**Error: "pthread not found"**
```bash
# Usually fixed by installing build-essential on Linux
sudo apt-get install build-essential
```

**Error: "Paho MQTT libraries not found"**
```bash
# Build script should fetch them
# Or manually check externallib/install directory
bash fetch_and_build.sh
```

### Runtime Issues

**Error: "config.json not found"**
```bash
# Ensure config.json exists in the working directory
# Or update the path in main.cpp:
# config.load("path/to/config.json");
```

**Error: "Cannot create socket" (MQTT)**
```bash
# Verify MQTT broker is running
# Check firewall settings
# Update MQTT broker address in config.json
```

---

## Next Steps

1. **Read [USAGE.md](./USAGE.md)** for comprehensive usage guide
2. **Review [ARCHITECTURE.md](./ARCHITECTURE.md)** for system design
3. **Check [EXAMPLES.md](./EXAMPLES.md)** for code examples
4. **Read [CONFIGURATION.md](./CONFIGURATION.md)** for configuration options

---

## System Requirements Summary

| Component | Requirement | Notes |
|-----------|-------------|-------|
| OS | Linux, macOS, Windows | Any recent version |
| CPU | x86_64, ARM | Any modern processor |
| RAM | 512 MB | Minimum; 2 GB+ recommended |
| Disk | 500 MB | For build and dependencies |
| C++ Compiler | C++17 support | GCC 7+, Clang 5+, MSVC 2017+ |
| CMake | 3.15+ | Cross-platform build tool |
| OpenSSL | 1.1.1+ | For MQTT security |

---

## Getting Help

- Check [TROUBLESHOOTING.md](./TROUBLESHOOTING.md) for common issues
- Review CMake output for detailed error messages
- Ensure all prerequisites are installed
- Check file permissions and paths
- Consult platform-specific documentation

---

**Installation complete!** Proceed to [USAGE.md](./USAGE.md) to learn how to use VirtualBus.
