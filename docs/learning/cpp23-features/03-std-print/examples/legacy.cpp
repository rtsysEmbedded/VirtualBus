// Legacy (pre-C++23) formatted output: printf (type-unsafe) and iostream
// (verbose, stateful manipulators).
// Compile: g++ -std=c++17 -Wall -Wextra -O2 legacy.cpp -o legacy_print
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    // --- printf: compiles cleanly despite a type mismatch (%d vs double) --
    // This is undefined behavior at runtime; only -Wformat catches it, and
    // only because GCC/Clang special-case printf-family functions.
    double batteryVoltage = 12.634;
    std::printf("Battery voltage: %d\n", batteryVoltage); // BUG: wrong specifier

    // Correct printf usage still needs manual specifier bookkeeping.
    std::printf("Battery voltage: %.2f V\n", batteryVoltage);

    // --- iostream: verbose for anything beyond default formatting ---------
    std::cout << "Node ID: " << std::hex << std::showbase << 0x2A << '\n';

    // Manipulator state leaks into subsequent, unrelated output: the next
    // integer is now also printed in hex unless explicitly reset.
    int nextValue = 42;
    std::cout << "Next value (accidentally still hex): " << nextValue << '\n';
    std::cout << std::dec; // must remember to reset manually

    // Fixed-width formatted table: requires several chained manipulators.
    std::cout << std::setw(10) << std::left << "Name"
              << std::setw(8) << std::right << "Value" << '\n';
    std::cout << std::setw(10) << std::left << "Voltage"
              << std::setw(8) << std::right << std::fixed << std::setprecision(2)
              << batteryVoltage << '\n';

    return 0;
}
