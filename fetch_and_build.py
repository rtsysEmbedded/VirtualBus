#!/usr/bin/env python3
import os
import subprocess
import platform
from pathlib import Path

class BuildSystem:
    def __init__(self):
        # Cross compilation settings
        self.cross_compile_path = "/home/ubuntu/Tools/arm-none-linux-gnueabihf-10.02/bin/"
        self.cross_compile = "arm-none-linux-gnueabihf-"
        
        # Set compilers
        self.cc = os.path.join(self.cross_compile_path, f"{self.cross_compile}gcc")
        self.cxx = os.path.join(self.cross_compile_path, f"{self.cross_compile}g++")
        self.ld = os.path.join(self.cross_compile_path, f"{self.cross_compile}ld")
    
    # Get the absolute path of the project directory (where fetch_and_build.py is located)
        self.project_path = Path(os.path.dirname(os.path.abspath(__file__)))
        print(f"Project path detected as: {self.project_path}")
    
    # Directory setup
        self.external_dir = self.project_path / "externallib"
        self.install_dir = self.external_dir / "install"
        self.mqtt_c_dir = self.external_dir / "paho.mqtt.c"
        self.mqtt_cpp_dir = self.external_dir / "paho.mqtt.cpp"
        self.spdlog_dir = self.external_dir / "spdlog"
        self.openssl_dir = self.external_dir / "openssl"
        self.canopen_linux_dir = self.external_dir / "canopenlinux"
        self.build_type = "Release"

    def run_command(self, cmd, cwd=None):
        """Execute shell command and handle errors"""
        try:
            subprocess.run(cmd, check=True, shell=True, cwd=cwd)
        except subprocess.CalledProcessError as e:
            print(f"Error executing command: {cmd}")
            print(f"Error: {str(e)}")
            exit(1)

    def create_directories(self):
        """Create necessary directories"""
        self.external_dir.mkdir(parents=True, exist_ok=True)
        self.install_dir.mkdir(parents=True, exist_ok=True)

    def build_openssl(self):
        """Clone and build OpenSSL"""
        if not self.openssl_dir.exists():
            print("Cloning OpenSSL repository...")
            self.run_command(f"git clone [https://github.com/openssl/openssl.git](https://github.com/openssl/openssl.git) {self.openssl_dir}")
            os.chdir(self.openssl_dir)
            self.run_command("git checkout OpenSSL_1_1_1t")
            print("Building OpenSSL for cross-compilation...")
            # Add OpenSSL build commands here

    def build_mqtt_c(self):
        """Clone and build Paho MQTT C"""
        if not self.mqtt_c_dir.exists():
            print("Cloning Paho MQTT C repository...")
            self.run_command(f"git clone [https://github.com/eclipse/paho.mqtt.c.git](https://github.com/eclipse/paho.mqtt.c.git) {self.mqtt_c_dir}")
            # Add MQTT C build commands here

    def build_mqtt_cpp(self):
        """Clone and build Paho MQTT C++"""
        if not self.mqtt_cpp_dir.exists():
            print("Cloning Paho MQTT C++ repository...")
            self.run_command(f"git clone [https://github.com/eclipse/paho.mqtt.cpp.git](https://github.com/eclipse/paho.mqtt.cpp.git) {self.mqtt_cpp_dir}")
            # Add MQTT C++ build commands here

    def build_spdlog(self):
        """Clone and build spdlog"""
        if not self.spdlog_dir.exists():
            print("Cloning spdlog repository...")
            self.run_command(f"git clone [https://github.com/gabime/spdlog.git](https://github.com/gabime/spdlog.git) {self.spdlog_dir}")
            # Add spdlog build commands here

    def build_project(self):
        """Build the main project"""
        build_dir = self.project_path / "build"
        build_dir.mkdir(exist_ok=True)
        os.chdir(build_dir)
        self.run_command("cmake ..")
        self.run_command("make")

def main():
    builder = BuildSystem()
    builder.create_directories()
    builder.build_openssl()
    builder.build_mqtt_c()
    builder.build_mqtt_cpp()
    builder.build_spdlog()
    builder.build_project()

if __name__ == "__main__":
    main()