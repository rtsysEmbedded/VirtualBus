cmake_minimum_required(VERSION 3.15)

set(CMAKE_UTILITY_DIR "${CMAKE_SOURCE_DIR}/cmake")

#include(${CMAKE_SOURCE_DIR}/toolchain.cmake)

# Project name
project(CPPProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(USE_SPDLOG ON CACHE BOOL "enable spd log for logging")

set(INTERNAL_LIB_DIR "${CMAKE_SOURCE_DIR}/libs")
set(INTERNAL_INC_DIR "${INTERNAL_LIB_DIR}/unicore/include")

# Set paths to the local installation of Paho MQTT
set(EXTERNAL_INSTALL_DIR "${CMAKE_SOURCE_DIR}/externallib/install")
set(JSON_CPP_INCLUDE_DIRS "${INTERNAL_LIB_DIR}")
set(PAHO_MQTT_C_INCLUDE_DIRS "${EXTERNAL_INSTALL_DIR}/include")
set(PAHO_MQTT_C_LIBRARIES "${EXTERNAL_INSTALL_DIR}/lib/libpaho-mqtt3a.a")
set(PAHO_MQTT_CPP_LIBRARIES "${EXTERNAL_INSTALL_DIR}/lib/libpaho-mqttpp3.a")
set(SPDLOG_LIBRARIES "${EXTERNAL_INSTALL_DIR}/lib/libspdlog.a")
set(OPNELSSL_LIBRARIES "${EXTERNAL_INSTALL_DIR}/lib/libssl.a")
set(OPNELSSL_CRYPT_LIBRARIES "${EXTERNAL_INSTALL_DIR}/lib/libcrypto.a")
# Find OpenSSL
# find_package(OpenSSL REQUIRED)

# if(OpenSSL_FOUND)
#     message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
# else()
#     message(FATAL_ERROR "OpenSSL not found! Please install OpenSSL.")
# endif()


# Include directories for Paho MQTT
include_directories(${PAHO_MQTT_C_INCLUDE_DIRS})
include_directories(${JSON_CPP_INCLUDE_DIRS})
include_directories(${INTERNAL_INC_DIR})
# Add source files
file(GLOB SOURCES "src/*.cpp" "${INTERNAL_LIB_DIR}/unicore/src/*.cpp")

# Add the executable
add_executable(CPPProject ${SOURCES})

option(ENABLE_SPDLOG "Enable spdlog for logging" ON)

if(ENABLE_SPDLOG)
    target_compile_definitions(CPPProject PRIVATE USE_SPDLOG)
    message(STATUS "spdlog is enabled.")
else()
    message(STATUS "spdlog is disabled.")
endif()

# Path to the version file and script
set(VERSION_FILE ${CMAKE_SOURCE_DIR}/version.txt)
set(INCREMENT_SCRIPT ${CMAKE_SOURCE_DIR}/increment_version.py)

# Custom command to update the version
add_custom_command(
    OUTPUT ${VERSION_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "Updating version..."
    COMMAND ${CMAKE_COMMAND} -E touch ${VERSION_FILE}
    COMMAND ${CMAKE_COMMAND} -E env python3 ${INCREMENT_SCRIPT}
    COMMENT "Incrementing project version"
)

# Ensure version file is updated before build
add_custom_target(increment_version ALL DEPENDS ${VERSION_FILE})

# Add version to the project
file(READ ${VERSION_FILE} VERSION_NUMBER)
message(STATUS "Project Version: ${VERSION_NUMBER}")
target_compile_definitions(CPPProject PRIVATE PROJECT_VERSION="${VERSION_NUMBER}")

# Link with libraries
target_link_libraries(CPPProject
    PUBLIC
    ${PAHO_MQTT_CPP_LIBRARIES}
    ${PAHO_MQTT_C_LIBRARIES}
    ${SPDLOG_LIBRARIES}
    ${OPNELSSL_LIBRARIES}
    ${OPNELSSL_CRYPT_LIBRARIES}
    pthread
)
