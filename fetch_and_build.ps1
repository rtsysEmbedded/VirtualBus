# Set paths
$ExternalDir = "externallib"
$MqttCDir = "$ExternalDir\paho.mqtt.c"
$MqttCppDir = "$ExternalDir\paho.mqtt.cpp"
$BuildType = "Release"

# Create externallib directory if it doesn't exist
if (-Not (Test-Path $ExternalDir)) {
    New-Item -ItemType Directory -Path $ExternalDir
}

# Fetch and build Paho MQTT C library
if (-Not (Test-Path $MqttCDir)) {
    Write-Host "Cloning Paho MQTT C library..."
    git clone https://github.com/eclipse/paho.mqtt.c.git $MqttCDir
    Set-Location $MqttCDir
    git checkout v1.3.12
    Write-Host "Building Paho MQTT C library..."
    cmake -Bbuild -S. -DPAHO_BUILD_STATIC=ON -DPAHO_WITH_SSL=ON -DCMAKE_BUILD_TYPE=$BuildType
    cmake --build build --target install
    Set-Location ..
} else {
    Write-Host "Paho MQTT C library already exists. Skipping fetch."
}

# Fetch and build Paho MQTT C++ library
if (-Not (Test-Path $MqttCppDir)) {
    Write-Host "Cloning Paho MQTT C++ library..."
    git clone https://github.com/eclipse/paho.mqtt.cpp.git $MqttCppDir
    Set-Location $MqttCppDir
    git checkout v1.4.0
    Write-Host "Initializing submodules for Paho MQTT C++ library..."
    git submodule init
    git submodule update
    Write-Host "Building Paho MQTT C++ library..."
    cmake -Bbuild -S. -DPAHO_WITH_MQTT_C=ON -DPAHO_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=$BuildType
    cmake --build build --target install
    Set-Location ..
} else {
    Write-Host "Paho MQTT C++ library already exists. Skipping fetch."
}

Write-Host "External libraries built successfully."
