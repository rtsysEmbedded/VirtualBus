#!/bin/bash

# Set the absolute path to your cross-compilers
#CROSS_COMPILE_PATH=/home/ubuntu/Tools/arm-none-linux-gnueabihf-10.02/bin/     # Modify this to the correct path
#CROSS_COMPILE=arm-none-linux-gnueabihf-  # Modify this if your toolchain has a different prefix
CROSS_COMPILE_PATH=
CROSS_COMPILE=

# Set the compilers and linker explicitly
CC=${CROSS_COMPILE_PATH}${CROSS_COMPILE}gcc
CXX=${CROSS_COMPILE_PATH}${CROSS_COMPILE}g++
LD=${CROSS_COMPILE_PATH}${CROSS_COMPILE}ld        # Linker

# Set up the external library and project directories
#PROJECT_PATH="/home/ubuntu/Documents/cpp_project"
# Get the directory where the script is located
PROJECT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Project path detected as: ${PROJECT_PATH}"
EXTERNAL_DIR="$PROJECT_PATH/externallib"
INSTALL_DIR="$PROJECT_PATH/externallib/install"  # Ensure absolute path is used
MQTT_C_DIR="$EXTERNAL_DIR/paho.mqtt.c"
MQTT_CPP_DIR="$EXTERNAL_DIR/paho.mqtt.cpp"
SPDLOG_DIR="$EXTERNAL_DIR/spdlog"
OPENSSL_DIR="$EXTERNAL_DIR/openssl"
CANOPEN_LINUX_DIR="$EXTERNAL_DIR/canopenlinux"
BUILD_TYPE="Release"

# Create necessary directories
mkdir -p $EXTERNAL_DIR
mkdir -p $INSTALL_DIR
ls $EXTERNAL_DIR
rm -rf $OPENSSL_DIR

rm -rf $MQTT_C_DIR

rm -rf $MQTT_CPP_DIR

rm -rf $SPDLOG_DIR

ls $EXTERNAL_DIR
    # Check if OpenSSL directory exists, if not, clone and build it
# if [ ! -d "$OPENSSL_DIR" ]; then
#     echo "Cloning OpenSSL repository..."
#     git clone https://github.com/openssl/openssl.git $OPENSSL_DIR
#     cd $OPENSSL_DIR
#     # Optionally checkout a specific tag, for example v1.1.1
#     git checkout OpenSSL_1_1_1t
#     echo "Building OpenSSL for cross-compilation..."

#     export CC=$CROSS_COMPILE_PATH$CROSS_COMPILE"gcc"
#     export CXX=$CROSS_COMPILE_PATH$CROSS_COMPILE"g++"
#     export LD=$CROSS_COMPILE_PATH$CROSS_COMPILE"ld"
#     # Create build directory
#     #./Configure linux-armv4 --cross-compile-prefix=${CROSS_COMPILE_PATH}${CROSS_COMPILE} --prefix=${INSTALL_DIR} -march=armv7-a -mfpu=neon
#     ./Configure linux-x86_64 --prefix=${INSTALL_DIR}

#     echo "Building canopen library..."
#     make CC=$CC CXX=$CXX LD=$LD
#     make install
#     cd -
# else
#     echo "OpenSSL source already exists. Skipping fetch."saeid 
# fi


# Fetch and build Paho MQTT C library
if [ ! -d "$MQTT_C_DIR" ]; then
    echo "Cloning Paho MQTT C library..."
    git clone https://github.com/eclipse/paho.mqtt.c.git $MQTT_C_DIR
    cd $MQTT_C_DIR
    git checkout v1.3.12
    echo "Building Paho MQTT C library..."
    cmake -Bbuild -H. \
    -DPAHO_BUILD_STATIC=ON \
    -DPAHO_WITH_SSL=OFF \
    -DOPENSSL_ROOT_DIR=${INSTALL_DIR} \
    -DOPENSSL_LIBRARIES=${INSTALL_DIR}/lib \
    -DOPENSSL_INCLUDE_DIR=${INSTALL_DIR}/include \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
    -DCMAKE_TOOLCHAIN_FILE=${PROJECT_PATH}/cmake/toolchain.cmake
   # Build and install the project
    cmake --build build --config $BUILD_TYPE
    cmake --build build --target install --config $BUILD_TYPE
    cd -
else
    echo "Paho MQTT C library already exists. Skipping fetch."
fi

# Fetch and build Paho MQTT C++ library
if [ ! -d "$MQTT_CPP_DIR" ]; then
    echo "Cloning Paho MQTT C++ library..."
    git clone https://github.com/eclipse/paho.mqtt.cpp.git $MQTT_CPP_DIR
    cd $MQTT_CPP_DIR
    git checkout v1.4.0
    echo "Initializing submodules for Paho MQTT C++ library..."
    git submodule init
    git submodule update
    echo "Building Paho MQTT C++ library..."
    cmake -Bbuild -H. \
        -DPAHO_BUILD_STATIC=ON \
        -DPAHO_WITH_MQTT_C=ON \
        -DPAHO_WITH_SSL=OFF \
        -DOPENSSL_ROOT_DIR=${INSTALL_DIR} \
        -DOPENSSL_LIBRARIES=${INSTALL_DIR}/lib \
        -DOPENSSL_INCLUDE_DIR=${INSTALL_DIR}/include \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        -DCMAKE_TOOLCHAIN_FILE=${PROJECT_PATH}/cmake/toolchain.cmake \
   # Build and install the project
    cmake --build build --config $BUILD_TYPE
    cmake --build build --target install --config $BUILD_TYPE
    cd -
else
    echo "Paho MQTT C++ library already exists. Skipping fetch."
fi

# Fetch and build Paho MQTT C++ library
if [ ! -d "$SPDLOG_DIR" ]; then
    echo "Cloning spdlog library..."
    git clone https://github.com/gabime/spdlog.git $SPDLOG_DIR
    cd $SPDLOG_DIR
    echo "Initializing submodules for spdlog library..."
    git submodule init
    git submodule update
    echo "Building spdlog library..."
    cmake -Bbuild -H. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR\
        -DCMAKE_TOOLCHAIN_FILE=${PROJECT_PATH}/cmake/toolchain.cmake \
   # Build and install the project
    cmake --build build --config $BUILD_TYPE
    cmake --build build --target install --config $BUILD_TYPE
    cd -
else
    echo "spdlog library already exists. Skipping fetch."
fi



echo "External libraries built and installed in $INSTALL_DIR."


# Fetch and build Paho MQTT C++ library
if [ ! -d "$CANOPEN_LINUX_DIR" ]; then
    echo "Cloning spdlog library..."
    git clone https://github.com/CANopenNode/CANopenLinux.git $CANOPEN_LINUX_DIR
    cd $CANOPEN_LINUX_DIR
    echo "Initializing submodules for spdlog library..."
    git submodule init
    git submodule update
    # Set up the environment variables for cross-compilation
    export CC=$CROSS_COMPILE_PATH$CROSS_COMPILE"gcc"
    export CXX=$CROSS_COMPILE_PATH$CROSS_COMPILE"g++"
    export LD=$CROSS_COMPILE_PATH$CROSS_COMPILE"ld"
    echo "Building canopen library..."
    make CC=$CC CXX=$CXX LD=$LD
    cd -
else
    echo "canopen library already exists. Skipping fetch."
fi