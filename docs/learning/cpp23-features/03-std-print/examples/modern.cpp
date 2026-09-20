// C++23: std::print / std::println - compile-time checked, stateless,
// writes directly to the target stream without an intermediate std::string.
// Compile: g++ -std=c++23 -Wall -Wextra -O2 modern.cpp -o modern_print
#include <print>

int main()
{
    double batteryVoltage = 12.634;

    // The line below would FAIL TO COMPILE (not just warn) if uncommented,
    // because {} count vs argument count is validated at compile time:
    //     std::println("Battery voltage: {} {}", batteryVoltage);

    std::println("Battery voltage: {:.2f} V", batteryVoltage);

    // Formatting is scoped to a single call - no manipulator state leaks
    // into the next statement.
    std::println("Node ID: {:#x}", 0x2A);
    std::println("Next value (unaffected by previous formatting): {}", 42);

    // Fixed-width table formatting reads left-to-right like the output.
    std::println("{:<10}{:>8}", "Name", "Value");
    std::println("{:<10}{:>8.2f}", "Voltage", batteryVoltage);

    return 0;
}
