// C++23: deducing this collapses the four overloads into one template,
// removes the templated CRTP base, and enables heap-allocation-free
// recursive lambdas.
// Compile: g++ -std=c++23 -Wall -Wextra -O2 modern.cpp -o modern_deducing_this
#include <iostream>
#include <string>
#include <utility>

// --- (1) A single member template replaces all four cv/ref overloads -------
struct Widget {
    std::string value;

    template <typename Self>
    auto&& get(this Self&& self)
    {
        std::cout << "[deduced Self]\n";
        return std::forward<Self>(self).value;
    }
};

// --- (2) CRTP without templating the base class -----------------------------
struct ShapeBase {
    template <typename Self>
    double area(this const Self& self)
    {
        // No static_cast<Derived*>(this) needed: Self is deduced directly
        // from the actual derived object at the call site.
        return self.areaImpl();
    }
};

struct Square : ShapeBase {
    double side;
    double areaImpl() const { return side * side; }
};

int main()
{
    Widget w{"hello"};
    const Widget cw{"world"};

    w.get();                 // deduces Self = Widget&
    cw.get();                // deduces Self = const Widget&
    Widget{"temp"}.get();    // deduces Self = Widget

    // --- (3) Recursive lambda: no std::function, no heap allocation --------
    auto factorial = [](this auto&& self, int n) -> int {
        return n <= 1 ? 1 : n * self(n - 1);
    };
    std::cout << "5! = " << factorial(5) << '\n';

    Square sq{{}, 4.0};
    std::cout << "Square area (deducing-this CRTP): " << sq.area() << '\n';

    return 0;
}
