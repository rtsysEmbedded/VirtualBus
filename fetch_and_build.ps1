# Get the script's directory path
$ProjectPath = $PSScriptRoot
Write-Host "Project path detected as: $ProjectPath"

# Set paths
$ExternalDir = Join-Path $ProjectPath "externallib"
$InstallDir = Join-Path $ExternalDir "install"
$MqttCDir = Join-Path $ExternalDir "paho.mqtt.c"
$MqttCppDir = Join-Path $ExternalDir "paho.mqtt.cpp"
$SpdlogDir = Join-Path $ExternalDir "spdlog"
$OpenSSLDir = Join-Path $ExternalDir "openssl"
$BuildType = "Release"

# Create directories
if (-Not (Test-Path $ExternalDir)) {
    New-Item -ItemType Directory -Path $ExternalDir
}
if (-Not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir
}

# Build OpenSSL
if (-Not (Test-Path $OpenSSLDir)) {
    Write-Host "Cloning OpenSSL repository..."
    git clone [https://github.com/openssl/openssl.git](https://github.com/openssl/openssl.git) $OpenSSLDir
    Set-Location $OpenSSLDir
    git checkout OpenSSL_1_1_1t
    Write-Host "Building OpenSSL..."
    # Add OpenSSL build commands here
    Set-Location $ProjectPath
}

# Build Paho MQTT C library
if (-Not (Test-Path $MqttCDir)) {
    Write-Host "Cloning Paho MQTT C library..."
    git clone [https://github.com/eclipse/paho.mqtt.c.git](https://github.com/eclipse/paho.mqtt.c.git) $MqttCDir
    Set-Location $MqttCDir
    git checkout v1.3.12
    Write-Host "Building Paho MQTT C library..."
    cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON -DPAHO_WITH_SSL=ON -DCMAKE_BUILD_TYPE=$BuildType
    cmake --build build --target install
    Set-Location $ProjectPath
} else {
    Write-Host "Paho MQTT C library already exists. Skipping fetch."
}

# Build Paho MQTT C++ library
if (-Not (Test-Path $MqttCppDir)) {
    Write-Host "Cloning Paho MQTT C++ library..."
    git clone [https://github.com/eclipse/paho.mqtt.cpp.git](https://github.com/eclipse/paho.mqtt.cpp.git) $MqttCppDir
    Set-Location $MqttCppDir
    git checkout v1.4.0
    Write-Host "Initializing submodules..."
    git submodule init
    git submodule update
    Write-Host "Building Paho MQTT C++ library..."
    cmake -Bbuild -H. -DPAHO_WITH_MQTT_C=ON -DPAHO_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=$BuildType
    cmake --build build --target install
    Set-Location $ProjectPath
} else {
    Write-Host "Paho MQTT C++ library already exists. Skipping fetch."
}

# Build spdlog
if (-Not (Test-Path $SpdlogDir)) {
    Write-Host "Cloning spdlog repository..."
    git clone [https://github.com/gabime/spdlog.git](https://github.com/gabime/spdlog.git) $SpdlogDir
    Set-Location $SpdlogDir
    Write-Host "Building spdlog..."
    cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=$BuildType
    cmake --build build --target install
    Set-Location $ProjectPath
}

# Build main project
Write-Host "Building main project..."
$BuildDir = Join-Path $ProjectPath "build"
if (-Not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir
}
Set-Location $BuildDir
cmake ..
cmake --build .

Write-Host "Build process completed."