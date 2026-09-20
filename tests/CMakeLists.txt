# Unit tests for VirtualBus.
#
# These build the parts of the library that have no external dependencies
# (no Paho MQTT, no spdlog, no OpenSSL) directly from source, so the test
# suite can build and run even in environments where externallib/ has not
# been fetched by fetch_and_build.sh -- which is also why it doesn't reuse
# the top-level CPPProject target.

set(UNICORE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs/unicore/include")
set(UNICORE_SRC_DIR "${CMAKE_SOURCE_DIR}/libs/unicore/src")
set(APP_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(NLOHMANN_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs")

add_library(unicore_core STATIC
    ${UNICORE_SRC_DIR}/VirtualBus.cpp
    ${UNICORE_SRC_DIR}/ThreadPool.cpp
    ${UNICORE_SRC_DIR}/ErrorHandler.cpp
    ${UNICORE_SRC_DIR}/VirtualBusCmd.cpp
    ${UNICORE_SRC_DIR}/Watchdog.cpp
)
target_include_directories(unicore_core PUBLIC ${UNICORE_INCLUDE_DIR} ${NLOHMANN_INCLUDE_DIR})
target_link_libraries(unicore_core PUBLIC pthread)

# InverterCommand.cpp and BatteryCommand.cpp live under src/ (application
# layer) rather than the unicore library, but SendTask/ReceiveTask depend
# on InverterCommand.cpp and the logger-propagation regression test
# exercises these classes directly.
add_library(app_commands STATIC
    ${APP_SRC_DIR}/InverterCommand.cpp
    ${APP_SRC_DIR}/BatteryCommand.cpp
)
target_include_directories(app_commands PUBLIC ${APP_SRC_DIR})
target_link_libraries(app_commands PUBLIC unicore_core)

# --- Regression test: TaskID must not violate the One Definition Rule ---
# See tests/odr/test_taskid_odr_main.cpp for the full explanation. tu_a.cpp
# and tu_b.cpp are separate translation units that both include Task.h;
# linking all three together is itself the regression test for the
# "multiple definition of TaskID::lastID_" bug.
add_executable(test_taskid_odr
    odr/tu_a.cpp
    odr/tu_b.cpp
    odr/test_taskid_odr_main.cpp
)
target_link_libraries(test_taskid_odr PRIVATE unicore_core)
target_include_directories(test_taskid_odr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_taskid_odr COMMAND test_taskid_odr)

# --- Regression test: SendTask/ReceiveTask must not shadow Task::logger_ ---
add_executable(test_logger_propagation test_logger_propagation.cpp)
target_link_libraries(test_logger_propagation PRIVATE unicore_core app_commands)
target_include_directories(test_logger_propagation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${APP_SRC_DIR})
add_test(NAME test_logger_propagation COMMAND test_logger_propagation)

# --- Regression test: BatteryCommand.h/BatteryCommandParser.h must
#     actually compile and work ---
# This file didn't compile at all before this fix (a header include cycle
# between BatteryCommand.h and BatteryCommandParser.h, a constructor that
# referenced a CommandType::Server enumerator that doesn't exist, a print()
# that called a print_base() that doesn't exist, and a parser that called
# setter methods BatteryStateCmd never declared), and was never included
# or built anywhere in the project, so none of that was ever caught. This
# target exercises the full parse -> set -> get round trip so a
# regression here is a build failure or a test failure, not silence.
add_executable(test_battery_command test_battery_command.cpp)
target_link_libraries(test_battery_command PRIVATE unicore_core app_commands)
target_include_directories(test_battery_command PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${APP_SRC_DIR})
add_test(NAME test_battery_command COMMAND test_battery_command)

# --- Basic VirtualBus smoke/feature tests (broadcast, targeted delivery,
#     priority ordering, clock injection, attach validation) ---
add_executable(test_virtualbus_basic test_virtualbus_basic.cpp)
target_link_libraries(test_virtualbus_basic PRIVATE unicore_core)
target_include_directories(test_virtualbus_basic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_virtualbus_basic COMMAND test_virtualbus_basic)

# --- Regression test: ThreadPool must dispatch by priority, not plain FIFO,
#     and must bound its per-priority queues ---
add_executable(test_threadpool_priority test_threadpool_priority.cpp)
target_link_libraries(test_threadpool_priority PRIVATE unicore_core)
target_include_directories(test_threadpool_priority PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_threadpool_priority COMMAND test_threadpool_priority)

# --- Regression test: VirtualBus's per-task message queues must be bounded,
#     with reject-new overflow handling reported via ReturnType::BUSY ---
add_executable(test_virtualbus_overflow test_virtualbus_overflow.cpp)
target_link_libraries(test_virtualbus_overflow PRIVATE unicore_core)
target_include_directories(test_virtualbus_overflow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_virtualbus_overflow COMMAND test_virtualbus_overflow)

# --- Regression test: Watchdog liveness monitoring and its Task integration ---
add_executable(test_watchdog test_watchdog.cpp)
target_link_libraries(test_watchdog PRIVATE unicore_core)
target_include_directories(test_watchdog PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_watchdog COMMAND test_watchdog)

# --- Regression test: receiveMessage() must not keep a dangling reference
#     into a task's queue after a concurrent detach() erases it ---
# Compiled as a standalone binary with its own copy of VirtualBus.cpp/
# ThreadPool.cpp/ErrorHandler.cpp (rather than linking unicore_core) so the
# whole call path -- library code included -- is instrumented with
# AddressSanitizer when available. Reusing the plain, uninstrumented
# unicore_core static lib here would let the use-after-free slip past ASan
# undetected, since ASan only inserts checks around memory accesses in
# code that was itself compiled with -fsanitize=address.
add_executable(test_virtualbus_detach_race
    test_virtualbus_detach_race.cpp
    ${UNICORE_SRC_DIR}/VirtualBus.cpp
    ${UNICORE_SRC_DIR}/ThreadPool.cpp
    ${UNICORE_SRC_DIR}/ErrorHandler.cpp
)
target_include_directories(test_virtualbus_detach_race PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${UNICORE_INCLUDE_DIR})
target_link_libraries(test_virtualbus_detach_race PRIVATE pthread)

# check_cxx_compiler_flag() alone isn't reliable here: it can report the
# flag as "supported" based on the compile step while the try-link still
# fails with undefined references to __asan_init (missing -fsanitize on
# the link line). Set CMAKE_REQUIRED_LINK_OPTIONS explicitly so the check
# actually exercises the link.
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-fsanitize=address,undefined")
set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address,undefined")
check_cxx_source_compiles("int main() { return 0; }" VB_HAS_ASAN)
unset(CMAKE_REQUIRED_FLAGS)
unset(CMAKE_REQUIRED_LINK_OPTIONS)
if(VB_HAS_ASAN AND NOT MSVC)
    target_compile_options(test_virtualbus_detach_race PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g -O0)
    target_link_options(test_virtualbus_detach_race PRIVATE -fsanitize=address,undefined)
else()
    message(WARNING "AddressSanitizer is not available: test_virtualbus_detach_race will still run, but is less likely to reliably catch a reintroduced use-after-free.")
endif()

add_test(NAME test_virtualbus_detach_race COMMAND test_virtualbus_detach_race)
