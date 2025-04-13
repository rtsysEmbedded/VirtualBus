#!/usr/bin/python3
import os
import subprocess


EXTERNAL_DIR = "externallib"
MQTT_C_DIR = os.path.join(EXTERNAL_DIR, "paho.mqtt.c")
MQTT_CPP_DIR = os.path.join(EXTERNAL_DIR, "paho.mqtt.cpp")
BUILD_TYPE = "Release"

def run_command(command, cwd=None):
    print(f"Running: {' '.join(command)}")
    subprocess.check_call(command, cwd=cwd, shell=True)

# Ensure externallib directory exists
os.makedirs(EXTERNAL_DIR, exist_ok=True)

# Fetch and build Paho MQTT C library
if not os.path.exists(MQTT_C_DIR):
    run_command(["git", "clone", "https://github.com/eclipse/paho.mqtt.c.git", MQTT_C_DIR])
    run_command(["git", "checkout", "v1.3.12"], cwd=MQTT_C_DIR)
    run_command(["cmake", "-Bbuild", "-H.", f"-DPAHO_BUILD_STATIC=ON", f"-DPAHO_WITH_SSL=ON", f"-DCMAKE_BUILD_TYPE={BUILD_TYPE}"], cwd=MQTT_C_DIR)
    run_command(["cmake", "--build", "build", "--target", "install"], cwd=MQTT_C_DIR)
else:
    print("Paho MQTT C library already exists. Skipping fetch.")

# Fetch and build Paho MQTT C++ library
if not os.path.exists(MQTT_CPP_DIR):
    run_command(["git", "clone", "https://github.com/eclipse/paho.mqtt.cpp.git", MQTT_CPP_DIR])
    run_command(["git", "checkout", "v1.4.0"], cwd=MQTT_CPP_DIR)
    run_command(["git", "submodule", "init"], cwd=MQTT_CPP_DIR)
    run_command(["git", "submodule", "update"], cwd=MQTT_CPP_DIR)
    run_command(["cmake", "-Bbuild", "-H.", f"-DPAHO_WITH_MQTT_C=ON", f"-DPAHO_BUILD_EXAMPLES=OFF", f"-DCMAKE_BUILD_TYPE={BUILD_TYPE}"], cwd=MQTT_CPP_DIR)
    run_command(["cmake", "--build", "build", "--target", "install"], cwd=MQTT_CPP_DIR)
else:
    print("Paho MQTT C++ library already exists. Skipping fetch.")

print("External libraries built successfully.")
