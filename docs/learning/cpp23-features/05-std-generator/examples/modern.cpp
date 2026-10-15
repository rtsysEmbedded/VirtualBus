// C++23: std::generator turns a coroutine using co_yield into a lazy,
// infinite-capable range, with automatic state management by the compiler.
// Compile: g++ -std=c++23 -fcoroutines -Wall -Wextra -O2 modern.cpp -o modern_generator
#include <generator>
#include <iostream>
#include <ranges>

// An infinite sequence expressed naturally - the caller decides how many
// values to actually consume. No manual state class is required; the
// compiler generates the coroutine frame that holds a, b and the
// suspend/resume point automatically.
std::generator<unsigned long long> fibonacci()
{
    unsigned long long a = 0, b = 1;
    while (true) {
        co_yield a;
        unsigned long long next = a + b;
        a = b;
        b = next;
    }
}

int main()
{
    std::cout << "Lazy generator (first 10): ";
    for (unsigned long long v : fibonacci() | std::views::take(10)) {
        std::cout << v << ' ';
    }
    std::cout << '\n';

    // Direct composition with <ranges> adaptors - filtering an infinite
    // sequence is trivial and still only computes what is consumed.
    std::cout << "Even Fibonacci numbers (first 5): ";
    for (unsigned long long v : fibonacci()
                                     | std::views::filter([](unsigned long long x) { return x % 2 == 0; })
                                     | std::views::take(5)) {
        std::cout << v << ' ';
    }
    std::cout << '\n';

    return 0;
}
