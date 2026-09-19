# Performance benchmarks for VirtualBus.
#
# Unlike tests/, these are measurement tools, not correctness checks:
# results are machine-dependent numbers, not pass/fail assertions, so
# they are intentionally NOT registered with add_test()/ctest. Build and
# run them manually:
#
#   cmake --build . --target bench_latency
#   ./benchmarks/bench_latency
#
# See RESULTS.md for the recorded baseline and how to record a new run
# after a change.

set(UNICORE_SRC_DIR "${CMAKE_SOURCE_DIR}/libs/unicore/src")
set(UNICORE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs/unicore/include")

# Built from its own copy of the unicore sources it needs (rather than
# linking the unicore_core static library from tests/CMakeLists.txt) so
# it can pin its own optimization flags below, independent of whatever
# unicore_core happened to be compiled with.
add_executable(bench_latency
    bench_latency.cpp
    ${UNICORE_SRC_DIR}/VirtualBus.cpp
    ${UNICORE_SRC_DIR}/ThreadPool.cpp
    ${UNICORE_SRC_DIR}/ErrorHandler.cpp
)
target_include_directories(bench_latency PRIVATE ${UNICORE_INCLUDE_DIR})
target_link_libraries(bench_latency PRIVATE pthread)

# Benchmark numbers are only comparable across commits/runs if the flags
# producing them don't silently drift with whatever CMAKE_BUILD_TYPE
# someone happens to configure the rest of the project with -- an
# unoptimized Debug build would make every latency number here
# meaningless. Pin -O2 regardless.
if(MSVC)
    target_compile_options(bench_latency PRIVATE /O2 /DNDEBUG)
else()
    target_compile_options(bench_latency PRIVATE -O2 -DNDEBUG)
endif()

# Stamp the binary with the commit it was built from, so a recorded run's
# stdout is self-describing even if it's saved to a file separately from
# this repository.
find_package(Git QUIET)
set(VB_BENCH_GIT_COMMIT "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VB_BENCH_GIT_COMMIT_RESULT
        RESULT_VARIABLE VB_BENCH_GIT_RESULT_CODE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(VB_BENCH_GIT_RESULT_CODE EQUAL 0 AND VB_BENCH_GIT_COMMIT_RESULT)
        set(VB_BENCH_GIT_COMMIT "${VB_BENCH_GIT_COMMIT_RESULT}")
    endif()
endif()
target_compile_definitions(bench_latency PRIVATE VB_BENCH_GIT_COMMIT="${VB_BENCH_GIT_COMMIT}")
