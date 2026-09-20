// Legacy (pre-C++23) lazy-sequence patterns: eager vector materialization,
// and a hand-written iterator that manually tracks generator state.
// Compile: g++ -std=c++17 -Wall -Wextra -O2 legacy.cpp -o legacy_generator
#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>

// --- Pattern A: eager materialization - computes everything up front -------
std::vector<unsigned long long> fibonacciEager(std::size_t count)
{
    std::vector<unsigned long long> result;
    result.reserve(count);
    unsigned long long a = 0, b = 1;
    for (std::size_t i = 0; i < count; ++i) {
        result.push_back(a);
        unsigned long long next = a + b;
        a = b;
        b = next;
    }
    return result; // cannot represent an infinite sequence at all
}

// --- Pattern B: hand-written iterator - explicit, manual state -------------
class FibonacciIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = unsigned long long;
    using difference_type = std::ptrdiff_t;
    using pointer = const unsigned long long*;
    using reference = unsigned long long;

    FibonacciIterator() = default; // end sentinel
    explicit FibonacciIterator(std::size_t remaining) : remaining_(remaining) {}

    unsigned long long operator*() const { return a_; }

    FibonacciIterator& operator++()
    {
        unsigned long long next = a_ + b_;
        a_ = b_;
        b_ = next;
        --remaining_;
        return *this;
    }

    bool operator!=(const FibonacciIterator& other) const { return remaining_ != other.remaining_; }

private:
    unsigned long long a_ = 0, b_ = 1;
    std::size_t remaining_ = 0;
};

class FibonacciRange {
public:
    explicit FibonacciRange(std::size_t count) : count_(count) {}
    FibonacciIterator begin() const { return FibonacciIterator(count_); }
    FibonacciIterator end() const { return FibonacciIterator(0); }

private:
    std::size_t count_;
};

int main()
{
    auto values = fibonacciEager(10);
    std::cout << "Eager (first 10): ";
    for (auto v : values) std::cout << v << ' ';
    std::cout << '\n';

    std::cout << "Manual iterator (first 10): ";
    for (auto v : FibonacciRange(10)) std::cout << v << ' ';
    std::cout << '\n';

    return 0;
}
