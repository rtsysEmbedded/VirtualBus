// C++23: std::expected replaces out-parameters / bare optional for
// fallible operations, and composes via and_then/transform/or_else.
// Compile: g++ -std=c++23 -Wall -Wextra -O2 modern.cpp -o modern_expected
#include <charconv>
#include <expected>
#include <iostream>
#include <string>
#include <string_view>

enum class ParseError { EmptyInput, NotANumber, OutOfRange };

std::expected<int, ParseError> parseInt(std::string_view text)
{
    if (text.empty()) {
        return std::unexpected(ParseError::EmptyInput);
    }
    int value = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec == std::errc::result_out_of_range) {
        return std::unexpected(ParseError::OutOfRange);
    }
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        return std::unexpected(ParseError::NotANumber);
    }
    return value;
}

std::expected<int, ParseError> doubleIfPositive(int value)
{
    if (value <= 0) {
        return std::unexpected(ParseError::OutOfRange);
    }
    return value * 2;
}

std::string_view describe(ParseError err)
{
    switch (err) {
        case ParseError::EmptyInput: return "empty input";
        case ParseError::NotANumber: return "not a number";
        case ParseError::OutOfRange: return "out of range";
    }
    return "unknown";
}

int main()
{
    // The compiler enforces attention: expected<T,E> is [[nodiscard]],
    // so the line below would produce a warning if left unused:
    //     parseInt("42");   // warning: ignoring return value ...

    auto result = parseInt("42");
    if (result.has_value()) {
        std::cout << "Parsed: " << *result << '\n';
    }

    // Monadic composition: chain fallible steps without nested if/else.
    auto chained = parseInt("21")
                       .and_then(doubleIfPositive)
                       .transform([](int v) { return v + 1; });

    if (chained) {
        std::cout << "Chained result: " << *chained << '\n';
    } else {
        std::cout << "Chained failed: " << describe(chained.error()) << '\n';
    }

    // The error type is explicit and carries the *reason*, unlike optional<int>.
    auto failure = parseInt("99999999999999999999");
    if (!failure) {
        std::cout << "Failure reason: " << describe(failure.error()) << '\n';
    }

    // or_else lets us provide a recovery path that itself can fail.
    auto recovered = parseInt("")
                          .or_else([](ParseError) -> std::expected<int, ParseError> {
                              return 0; // default fallback value
                          });
    std::cout << "Recovered value: " << *recovered << '\n';

    return 0;
}
