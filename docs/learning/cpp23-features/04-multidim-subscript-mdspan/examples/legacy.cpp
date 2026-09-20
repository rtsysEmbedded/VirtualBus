// Legacy (pre-C++23) approaches to 2D data: vector-of-vectors (heap churn,
// non-contiguous memory) and manual flat-index arithmetic (no type safety).
// Compile: g++ -std=c++17 -Wall -Wextra -O2 legacy.cpp -o legacy_mdspan
#include <cstddef>
#include <iostream>
#include <vector>

// --- Pattern A: vector<vector<T>> - one heap allocation per row ------------
class MatrixOfVectors {
public:
    MatrixOfVectors(std::size_t rows, std::size_t cols)
        : data_(rows, std::vector<double>(cols, 0.0)) {}

    double& at(std::size_t row, std::size_t col) { return data_[row][col]; }

private:
    std::vector<std::vector<double>> data_; // rows are NOT contiguous in memory
};

// --- Pattern B: manual flat-index arithmetic - fast but not type-safe ------
class FlatMatrix {
public:
    FlatMatrix(std::size_t rows, std::size_t cols)
        : rows_(rows), cols_(cols), data_(rows * cols, 0.0) {}

    // Easy to accidentally swap row/col or get the stride formula wrong;
    // the compiler cannot catch either mistake.
    double& at(std::size_t row, std::size_t col) { return data_[row * cols_ + col]; }

private:
    std::size_t rows_, cols_;
    std::vector<double> data_;
};

int main()
{
    MatrixOfVectors mv(3, 3);
    mv.at(1, 1) = 5.0;
    std::cout << "vector<vector<double>> [1][1] = " << mv.at(1, 1) << '\n';

    FlatMatrix fm(3, 3);
    fm.at(1, 1) = 5.0;
    // BUG risk: nothing stops a caller from writing fm.at(col, row) by mistake.
    std::cout << "Flat-index matrix (1,1) = " << fm.at(1, 1) << '\n';

    return 0;
}
