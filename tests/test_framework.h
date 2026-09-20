// Minimal, dependency-free test harness for VirtualBus's unit tests.
//
// The project had no test framework wired in (tests/test_main.cpp was a
// placeholder that printed a string and asserted nothing). Pulling in
// GoogleTest would add a network-fetch dependency to the build that this
// environment can't reliably guarantee, so this header provides just
// enough machinery -- VB_TEST to register a test function, VB_CHECK to
// assert inside it -- to write and run real regression tests without any
// external dependency.
#ifndef VIRTUALBUS_TEST_FRAMEWORK_H
#define VIRTUALBUS_TEST_FRAMEWORK_H

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace vbtest {

inline int& failureCount() {
    static int count = 0;
    return count;
}

inline void checkImpl(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::cerr << "[FAIL] " << file << ":" << line << ": CHECK(" << expr << ") failed\n";
        ++failureCount();
    }
}

using TestFn = std::function<void()>;

struct TestCase {
    std::string name;
    TestFn fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const std::string& name, TestFn fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline int runAll() {
    int total = 0;
    for (auto& test : registry()) {
        std::cout << "[ RUN ] " << test.name << '\n';
        int before = failureCount();
        try {
            test.fn();
        } catch (const std::exception& e) {
            std::cerr << "[FAIL] " << test.name << " threw std::exception: " << e.what() << '\n';
            ++failureCount();
        } catch (...) {
            std::cerr << "[FAIL] " << test.name << " threw an unknown exception\n";
            ++failureCount();
        }
        ++total;
        std::cout << (failureCount() == before ? "[  OK ] " : "[FAIL ] ") << test.name << '\n';
    }
    std::cout << total << " test(s) run, " << failureCount() << " failure(s).\n";
    return failureCount() == 0 ? 0 : 1;
}

} // namespace vbtest

#define VB_CHECK(cond) ::vbtest::checkImpl((cond), #cond, __FILE__, __LINE__)

#define VB_TEST(name)                                                  \
    static void name();                                                \
    static ::vbtest::Registrar registrar_##name(#name, &name);          \
    static void name()

#endif // VIRTUALBUS_TEST_FRAMEWORK_H
