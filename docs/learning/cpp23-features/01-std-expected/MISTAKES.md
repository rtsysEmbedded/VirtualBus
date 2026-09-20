# Common Mistakes — What Goes Wrong and How to Fix It

Learn from these real errors. Try to break each pattern intentionally to understand the fixes.

---

## Mistake 1: Forgetting to Check for Failure

### The problem

```cpp
auto result = parseConfig("file.conf");
int value = *result;  // BUG: crashes if parsing failed
```

**What happens:** if `result` contains an error, dereferencing it with `*result` is **undefined behavior** — typically a crash (segfault, memory access violation).

### The fix

Always check before dereferencing:

```cpp
auto result = parseConfig("file.conf");

// Option 1: check has_value()
if (result.has_value()) {
    int value = *result;
    // ... use value ...
}

// Option 2: use value_or() with a default
int value = result.value_or(0);  // returns 0 if error

// Option 3: explicit error handling
if (result) {
    int value = *result;
    // ... use value ...
} else {
    std::cerr << "Error: " << result.error() << '\n';
}
```

**Remember:** `std::expected` is marked `[[nodiscard]]`, so not checking triggers a compiler warning (if enabled with `-Wunused-result`). Always enable this warning.

---

## Mistake 2: Accessing `.error()` When There Is No Error

### The problem

```cpp
auto result = parseConfig("file.conf");
if (result) {
    std::cout << result.error() << '\n';  // BUG: no error to access
}
```

**What happens:** calling `.error()` when `has_value() == true` is **undefined behavior**. The program may crash or produce garbage.

### The fix

Always check that the value is absent before accessing the error:

```cpp
auto result = parseConfig("file.conf");

if (!result) {  // explicitly check for error
    std::cout << result.error() << '\n';  // safe now
}

// or use value_or() to provide a default
int val = result.value_or(0);  // never crashes
```

---

## Mistake 3: Incorrect `.value()` vs. `*` vs. `.value_or()`

### The problem

```cpp
auto result = parseConfig("file.conf");

int a = *result;           // crashes if has_value() == false
int b = result.value();    // throws std::bad_expected_access if has_value() == false
int c = result.value_or(0);  // returns 0 if has_value() == false (SAFE)
```

### The fix

Understand the difference:

- **`*result`** — dereference, like a pointer. Fast, but crashes on error. Use only when you **know** it has a value.
- **`result.value()`** — throws an exception on error. Useful in exception-heavy code, but `-fno-exceptions` incompatible.
- **`result.value_or(default)`** — safe, returns a default if error. Best for most cases.

Use this table:

| Situation | Use | Reason |
|-----------|-----|--------|
| "I know it has a value" | `*result` | Fastest, no overhead |
| "Let it throw if wrong" | `result.value()` | Exception-based error handling |
| "Provide a sensible default" | `result.value_or(0)` | No crashes, no exceptions, safest |

---

## Mistake 4: Wrong Error Type in Composition

### The problem

```cpp
std::expected<int, ParseError> step1() { /* ... */ }
std::expected<int, IOError> step2() { /* ... */ }  // Different error type!

auto result = step1().and_then([](int x) {
    return step2();  // ERROR: returns IOError, not ParseError
});
```

**What happens:** the two functions use different error types. Composition fails because the compiler cannot mix them.

### The fix

Use `transform_error()` to convert one type to another:

```cpp
auto result = step1()
    .and_then([](int x) {
        return step2()
            .transform_error([](IOError e) -> ParseError {
                // Convert IOError to ParseError
                return ParseError::IOFailed;
            });
    });
```

Or better: **use a unified error type from the start**:

```cpp
enum class AppError {
    ParseFailed,
    IOFailed,
    ValidationFailed
};

std::expected<int, AppError> step1() { /* ... */ }
std::expected<int, AppError> step2() { /* ... */ }  // Same error type

auto result = step1().and_then([](int x) {
    return step2();  // Works perfectly
});
```

---

## Mistake 5: Forgetting to `return std::unexpected()`

### The problem

```cpp
std::expected<int, ParseError> parse(const std::string& s) {
    if (s.empty()) {
        ParseError::EmptyInput;  // BUG: not returning!
    }
    return 42;
}
```

**What happens:** the error value is created but not returned. The function continues and returns success (42) even though parsing failed.

### The fix

Use `return std::unexpected()`:

```cpp
std::expected<int, ParseError> parse(const std::string& s) {
    if (s.empty()) {
        return std::unexpected(ParseError::EmptyInput);  // CORRECT
    }
    return 42;
}
```

---

## Mistake 6: Implicit Conversion of Success Values

### The problem

```cpp
std::expected<int, ParseError> parse(const std::string& s) {
    return "not an int";  // BUG: std::string is not int
}
```

**What happens:** compilation error. `std::expected<int, E>` expects the success value to be `int`, not `std::string`.

### The fix

Be explicit about the type:

```cpp
std::expected<int, ParseError> parse(const std::string& s) {
    return 42;  // Correct type
}

std::expected<std::string, ParseError> parseString(const std::string& s) {
    return s;  // Different return type
}
```

---

## Mistake 7: Chaining Without `and_then`

### The problem

```cpp
auto result = step1()
    .transform([](int x) {
        return step2();  // BUG: step2 returns std::expected, not int
    });

// result is now std::expected<std::expected<int, E>, E> — nested!
```

**What happens:** `.transform()` wraps the return value in another layer of `std::expected`. You get `std::expected<std::expected<int, E>, E>` instead of the flat `std::expected<int, E>`.

### The fix

Use `.and_then()` instead of `.transform()`:

```cpp
auto result = step1()
    .and_then([](int x) {
        return step2();  // CORRECT: step2 already returns std::expected
    });

// result is std::expected<int, E> — flat
```

**Rule of thumb:**
- Use `.transform()` if the lambda returns a plain value (not `std::expected`)
- Use `.and_then()` if the lambda returns `std::expected<T, E>`

---

## Mistake 8: Ignoring Error Context

### The problem

```cpp
std::expected<Config, std::string> loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected("error");  // Too vague!
    }
    // ...
}
```

**What happens:** the error message "error" doesn't tell you what went wrong (file not found? permission denied? disk full?). Debugging is hard.

### The fix

Be specific and include context:

```cpp
std::expected<Config, std::string> loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected("failed to open " + path + ": file not found");
    }
    // ...
}

// Or use a struct for richer errors:
struct LoadError {
    std::string message;
    std::string filename;
    int errno_value;
};

std::expected<Config, LoadError> loadConfigRich(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(LoadError{
            .message = "failed to open",
            .filename = path,
            .errno_value = errno
        });
    }
    // ...
}
```

---

## Mistake 9: Mixing `std::expected` and `std::optional`

### The problem

```cpp
std::expected<std::optional<int>, ParseError> parse(const std::string& s) {
    // Confusing: what does "no value" mean? Success with nothing? Or failure?
    if (s == "none") {
        return std::optional<int>();  // Success, but no value
    }
    if (s.empty()) {
        return std::unexpected(ParseError::Empty);  // Failure
    }
    return std::stoi(s);
}
```

**What happens:** it works, but is confusing. Readers don't know if "no value" means success or failure.

### The fix

Be explicit:

```cpp
// If the value *might* be absent on success, use optional:
std::expected<std::optional<int>, ParseError> parse(const std::string& s) {
    if (s == "none") {
        return std::optional<int>();  // Success with nothing
    }
    return std::stoi(s);  // Success with a value
}

// If a value is always required or it's an error, don't use optional:
std::expected<int, ParseError> parse(const std::string& s) {
    if (s == "none") {
        return std::unexpected(ParseError::NoValue);  // Failure
    }
    return std::stoi(s);  // Success
}
```

**Rule:** use `std::optional` inside `std::expected` only when the absence of a value is a **valid success state**. Otherwise, make it an error.

---

## Mistake 10: Not Testing Error Paths

### The problem

```cpp
std::expected<int, ParseError> parse(const std::string& s) {
    if (s.empty()) {
        return std::unexpected(ParseError::Empty);
    }
    return std::stoi(s);
}

// Test only the happy path:
int main() {
    auto result = parse("42");
    assert(result);  // This passes, but we never tested the error case!
}
```

**What happens:** bugs in error handling go undetected. Users find them in production.

### The fix

Test **both** paths:

```cpp
int main() {
    // Test success
    auto success = parse("42");
    assert(success.has_value());
    assert(*success == 42);
    
    // Test error
    auto failure = parse("");
    assert(!failure.has_value());
    assert(failure.error() == ParseError::Empty);
    
    // Test edge cases
    auto invalid = parse("abc");
    assert(!invalid.has_value());
}
```

Use a testing framework (like Google Test or Catch2) to make this automatic:

```cpp
#include <gtest/gtest.h>

TEST(ParseTest, SuccessCase) {
    auto result = parse("42");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 42);
}

TEST(ParseTest, EmptyInput) {
    auto result = parse("");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ParseError::Empty);
}

TEST(ParseTest, InvalidInput) {
    auto result = parse("abc");
    ASSERT_FALSE(result);
}
```

---

## Quick reference: Do's and Don'ts

| Do | Don't |
|---|---|
| Check before dereferencing (`if (result) ...`) | Dereference blindly (`*result` without checking) |
| Use `.and_then()` to chain fallible operations | Use `.transform()` for chaining (causes nesting) |
| Provide specific error messages | Use vague errors like "error" |
| Test both success and failure paths | Test only the happy path |
| Use `return std::unexpected()` | Forget the `return` keyword |
| Use a unified error enum type | Mix different error types |
| Use `value_or()` for defaults | Use `.value()` in error-prone code |

---

## Practice: Fix these bugs

Try to spot and fix the bugs in each snippet:

### Bug 1
```cpp
auto result = parse("hello");
int x = *result;  // FIX ME
std::cout << result.error() << '\n';  // FIX ME
```

### Bug 2
```cpp
std::expected<int, std::string> process(int x) {
    return "ok";  // FIX ME
}
```

### Bug 3
```cpp
auto result = step1()
    .transform([](int x) {
        return step2();  // FIX ME (step2 returns expected)
    });
```

### Bug 4
```cpp
std::expected<int, ParseError> parse(const std::string& s) {
    if (s.empty()) {
        ParseError::Empty;  // FIX ME
    }
    return std::stoi(s);
}
```

Answers are at the bottom. Try to fix them first!

---

## Answers

**Bug 1:**
```cpp
auto result = parse("hello");
if (result) {  // Check before using
    int x = *result;
    std::cout << "Success\n";
} else {
    std::cout << "Error: " << result.error() << '\n';
}
```

**Bug 2:**
```cpp
std::expected<int, std::string> process(int x) {
    return 42;  // Return the correct type (int, not string)
}
```

**Bug 3:**
```cpp
auto result = step1()
    .and_then([](int x) {  // Use and_then, not transform
        return step2();
    });
```

**Bug 4:**
```cpp
std::expected<int, ParseError> parse(const std::string& s) {
    if (s.empty()) {
        return std::unexpected(ParseError::Empty);  // Add return
    }
    return std::stoi(s);
}
```
