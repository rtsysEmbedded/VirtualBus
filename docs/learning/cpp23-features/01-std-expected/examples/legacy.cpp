// Legacy (pre-C++23) error handling patterns for a fallible parse operation.
// Compile: g++ -std=c++17 -Wall -Wextra -O2 legacy.cpp -o legacy_expected
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

enum class ParseError { EmptyInput, NotANumber, OutOfRange };

// --- Pattern A: C-style error code + out-parameter -------------------------
// Nothing in the type system forces the caller to check the bool result.
bool parseIntLegacyOutParam(const std::string& text, int& outValue, ParseError& outError)
{
    if (text.empty()) {
        outError = ParseError::EmptyInput;
        return false;
    }
    char* end = nullptr;
    errno = 0;
    long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') {
        outError = ParseError::NotANumber;
        return false;
    }
    if (errno == ERANGE || value < INT32_MIN || value > INT32_MAX) {
        outError = ParseError::OutOfRange;
        return false;
    }
    outValue = static_cast<int>(value);
    return true;
}

// --- Pattern B: std::optional loses the *reason* for failure ---------------
std::optional<int> parseIntLegacyOptional(const std::string& text)
{
    int value = 0;
    ParseError err{};
    if (!parseIntLegacyOutParam(text, value, err)) {
        return std::nullopt; // caller has no idea *why* it failed
    }
    return value;
}

int main()
{
    // Demonstrates the real-world footgun: the return value is silently
    // discarded and the compiler emits no warning at all with -std=c++17.
    int value = -1;
    ParseError error{};
    parseIntLegacyOutParam("not-a-number", value, error); // BUG: result ignored
    std::cout << "Legacy out-param call ignored its result. "
              << "'value' still holds its previous default: " << value << '\n';

    // Correct usage requires manual discipline every single call site.
    if (int v; parseIntLegacyOutParam("42", v, error)) {
        std::cout << "Parsed correctly: " << v << '\n';
    }

    // optional<int> tells us *that* it failed but not *why*.
    auto maybe = parseIntLegacyOptional("99999999999999999999");
    if (!maybe) {
        std::cout << "optional<int> failed, but the reason (OutOfRange) is lost.\n";
    }

    return 0;
}
