// Legacy (pre-C++23) patterns that "deducing this" was designed to replace:
// 1) four manual overloads to cover const/ref-qualifiers,
// 2) CRTP with a templated base class,
// 3) recursive lambda via std::function.
// Compile: g++ -std=c++17 -Wall -Wextra -O2 legacy.cpp -o legacy_deducing_this
#include <functional>
#include <iostream>
#include <string>
#include <utility>

// --- (1) Four overloads needed to correctly handle every value category ----
struct Widget {
    std::string value;

    std::string& get() &                 { std::cout << "[lvalue]\n";       return value; }
    const std::string& get() const &     { std::cout << "[const lvalue]\n"; return value; }
    std::string&& get() &&               { std::cout << "[rvalue]\n";       return std::move(value); }
    const std::string&& get() const &&   { std::cout << "[const rvalue]\n"; return std::move(value); }
};

// --- (2) CRTP: base class must be templated on the derived type ------------
template <typename Derived>
struct ShapeBase {
    double area() const
    {
        // Manual downcast; if Derived doesn't implement areaImpl(),
        // this fails with a deep, hard-to-read template error.
        return static_cast<const Derived*>(this)->areaImpl();
    }
};

struct Square : ShapeBase<Square> {
    double side;
    double areaImpl() const { return side * side; }
};

int main()
{
    Widget w{"hello"};
    const Widget cw{"world"};

    w.get();                 // -> [lvalue]
    cw.get();                // -> [const lvalue]
    Widget{"temp"}.get();    // -> [rvalue]

    // --- (3) Recursive lambda requires std::function (heap allocation) ----
    std::function<int(int)> factorial = [&factorial](int n) -> int {
        return n <= 1 ? 1 : n * factorial(n - 1);
    };
    std::cout << "5! = " << factorial(5) << '\n';

    Square sq{{}, 4.0};
    std::cout << "Square area (CRTP): " << sq.area() << '\n';

    return 0;
}
