# if(NOT CMAKE_C_COMPILER)
#     set(CMAKE_C_COMPILER "/path/to/gcc" CACHE STRING "C Compiler")
# endif()

# if(NOT CMAKE_CXX_COMPILER)
#     set(CMAKE_CXX_COMPILER "/path/to/g++" CACHE STRING "C++ Compiler")
# endif()

# message(STATUS "C Compiler set to: ${CMAKE_C_COMPILER}")
# message(STATUS "C++ Compiler set to: ${CMAKE_CXX_COMPILER}")


# Set the system name and architecture for cross-compiling
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set the absolute path to your cross-compilers
set(CROSS_COMPILE_PATH "/home/ubuntu/Tools/arm-none-linux-gnueabihf-10.02/bin/")
set(CROSS_COMPILE arm-none-linux-gnueabihf-)

# Set the compilers and linker explicitly
set(CMAKE_C_COMPILER ${CROSS_COMPILE_PATH}${CROSS_COMPILE}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE_PATH}${CROSS_COMPILE}g++)
set(CMAKE_LINKER ${CROSS_COMPILE_PATH}${CROSS_COMPILE}ld)

# Flags for cross-compiling (adjust if needed for your architecture)
set(CMAKE_C_FLAGS "-O2 -mfpu=vfpv3 -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS "-O2 -mfpu=vfpv3 -mfloat-abi=hard")

# Optionally, specify the sysroot (uncomment if needed)
# set(CMAKE_SYSROOT /path/to/your/sysroot)
