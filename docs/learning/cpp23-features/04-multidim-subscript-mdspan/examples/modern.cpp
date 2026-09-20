// C++23: std::mdspan as a non-owning, type-safe multidimensional view over
// a contiguous flat buffer, paired with the new multi-argument operator[].
// Compile: g++ -std=c++23 -Wall -Wextra -O2 modern.cpp -o modern_mdspan
#include <iostream>
#include <mdspan>
#include <vector>

int main()
{
    // Single contiguous allocation - the actual storage.
    std::vector<double> buffer(3 * 3, 0.0);

    // A view: no copy, no extra allocation, just pointer + extents metadata.
    // Extents<size_t, 3, 3> are compile-time constants here, so the compiler
    // can fully inline index arithmetic - identical codegen to manual math,
    // but with a type-checked, self-documenting call site.
    std::mdspan<double, std::extents<std::size_t, 3, 3>> view(buffer.data());

    view[1, 1] = 5.0; // new C++23 multi-argument operator[]
    std::cout << "mdspan[1, 1] = " << view[1, 1] << '\n';

    // Iterating with named extents instead of a hand-derived formula:
    for (std::size_t row = 0; row < view.extent(0); ++row) {
        for (std::size_t col = 0; col < view.extent(1); ++col) {
            view[row, col] = static_cast<double>(row * view.extent(1) + col);
        }
    }

    std::cout << "Matrix contents:\n";
    for (std::size_t row = 0; row < view.extent(0); ++row) {
        for (std::size_t col = 0; col < view.extent(1); ++col) {
            std::cout << view[row, col] << ' ';
        }
        std::cout << '\n';
    }

    // The underlying buffer is unchanged and still owns the memory -
    // mdspan is purely a view, exactly like std::span but multidimensional.
    std::cout << "Underlying buffer.size() = " << buffer.size() << '\n';

    return 0;
}
