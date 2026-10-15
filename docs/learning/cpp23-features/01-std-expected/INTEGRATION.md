# Integration Guide — Adding `std::expected` to Your Code

This guide shows you how to gradually adopt `std::expected` in existing projects without rewriting everything at once.

---

## Step 1: Start Small — Pick One Function

Don't convert your entire codebase at once. Pick one function that handles errors poorly:

### Before: Using exceptions

```cpp
// old_code.cpp
int parseConfigValue(const std::string& key) {
    auto cfg = loadConfig();  // throws on error
    auto value = cfg.at(key);  // throws if key not found
    return std::stoi(value);   // throws if not a number
}
```

**Problems:**
- Error path is invisible in the signature
- Caller must know to use try/catch
- No way to recover from errors gracefully

### After: Using `std::expected`

```cpp
// new_code.cpp
enum class ConfigError {
    FileNotFound,
    KeyNotFound,
    InvalidNumber
};

std::expected<int, ConfigError> parseConfigValue(const std::string& key) {
    auto cfg = loadConfig();
    if (!cfg) {
        return std::unexpected(ConfigError::FileNotFound);
    }
    
    auto value = cfg->at(key);
    if (!value) {
        return std::unexpected(ConfigError::KeyNotFound);
    }
    
    try {
        return std::stoi(*value);
    } catch (...) {
        return std::unexpected(ConfigError::InvalidNumber);
    }
}
```

**Benefits:**
- Error type is explicit in the signature
- Caller is forced to handle errors (compiler warning if ignored)
- Can chain operations with `.and_then()`

---

## Step 2: Wrap Old Functions

If you can't modify old code, wrap it:

### Before: Calling old exception-based code

```cpp
int value = 0;
try {
    value = legacyParse(input);  // throws
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return -1;
}
```

### After: Wrapped with `std::expected`

```cpp
std::expected<int, std::string> safeParse(const std::string& input) {
    try {
        return legacyParse(input);  // calls the old throwing function
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}

// Now use it cleanly:
auto result = safeParse(input);
if (result) {
    std::cout << "Parsed: " << *result << '\n';
} else {
    std::cout << "Error: " << result.error() << '\n';
}
```

This is a great pattern for incremental adoption: wrap old code, use new code.

---

## Step 3: Migrate Error Codes to `std::expected`

### Before: C-style error codes

```cpp
bool readFile(const std::string& path, std::string& output, int& errCode) {
    std::ifstream file(path);
    if (!file.is_open()) {
        errCode = 1;  // "file not found"
        return false;
    }
    
    try {
        std::getline(file, output);
        errCode = 0;
        return true;
    } catch (...) {
        errCode = 2;  // "read error"
        return false;
    }
}

// Usage: must check both return value and error code
std::string data;
int err;
if (!readFile("file.txt", data, err)) {
    if (err == 1) { /* handle file not found */ }
    if (err == 2) { /* handle read error */ }
}
```

**Problems:**
- Two separate return channels (bool + errCode)
- Easy to forget checking either one
- Error codes are magic numbers

### After: Using `std::expected`

```cpp
enum class FileError {
    NotFound,
    ReadError,
    PermissionDenied
};

std::expected<std::string, FileError> readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(FileError::NotFound);
    }
    
    std::string data;
    try {
        std::getline(file, data);
        return data;
    } catch (...) {
        return std::unexpected(FileError::ReadError);
    }
}

// Usage: single return channel, must check
auto result = readFile("file.txt");
if (result) {
    std::cout << *result << '\n';
} else {
    switch (result.error()) {
        case FileError::NotFound:
            std::cout << "File not found\n";
            break;
        case FileError::ReadError:
            std::cout << "Read error\n";
            break;
        case FileError::PermissionDenied:
            std::cout << "Permission denied\n";
            break;
    }
}
```

**Benefits:**
- Single return channel
- Error type is clear and typed
- No magic numbers
- Compiler forces you to check

---

## Step 4: Update the Call Site

### Before: Multiple error handling patterns

```cpp
int main() {
    // Pattern 1: out-parameter
    int value = 0;
    int err = 0;
    if (!parse(input, value, err)) {
        std::cerr << "Parse error: " << err << '\n';
        return 1;
    }
    
    // Pattern 2: exception handling
    try {
        processValue(value);
    } catch (const std::exception& e) {
        std::cerr << "Process error: " << e.what() << '\n';
        return 1;
    }
    
    // Pattern 3: optional (loses error info)
    auto result = validate(value);
    if (!result) {
        std::cerr << "Validation failed\n";
        return 1;
    }
    
    return 0;
}
```

### After: Uniform with `std::expected`

```cpp
enum class AppError {
    ParseFailed,
    ProcessFailed,
    ValidationFailed
};

int main() {
    // All patterns now use the same approach
    auto parsed = parse(input);
    if (!parsed) {
        std::cerr << "Parse error\n";
        return 1;
    }
    
    auto processed = processValue(*parsed);
    if (!processed) {
        std::cerr << "Process error\n";
        return 1;
    }
    
    auto validated = validate(*processed);
    if (!validated) {
        std::cerr << "Validation failed\n";
        return 1;
    }
    
    std::cout << "Success: " << *validated << '\n';
    return 0;
}
```

Or with composition:

```cpp
int main() {
    auto result = parse(input)
        .and_then(processValue)
        .and_then(validate);
    
    if (result) {
        std::cout << "Success: " << *result << '\n';
        return 0;
    } else {
        std::cerr << "Failed\n";
        return 1;
    }
}
```

---

## Step 5: Handle Multiple Error Types

If you have functions with different error types, unify them:

### Before: Mixed error types

```cpp
// Function 1: uses ParseError
std::expected<int, ParseError> parseNumber(const std::string& s);

// Function 2: uses FileError  
std::expected<std::string, FileError> readFile(const std::string& path);

// Can't easily compose them
```

### After: Unified error type

```cpp
enum class AppError {
    // ParseError variants
    ParseInvalid,
    ParseEmpty,
    
    // FileError variants
    FileNotFound,
    FileReadFailed
};

std::expected<int, AppError> parseNumber(const std::string& s) {
    // ... implementation ...
}

std::expected<std::string, AppError> readFile(const std::string& path) {
    // ... implementation ...
}

// Now composition works:
auto result = readFile("numbers.txt")
    .and_then([](const std::string& content) {
        return parseNumber(content);
    });
```

**Or: Keep separate error types, convert at boundaries:**

```cpp
std::expected<int, AppError> processConfig(const std::string& path) {
    return readFile(path)
        .transform_error([](FileError e) -> AppError {
            // Convert FileError to AppError
            switch (e) {
                case FileError::NotFound:
                    return AppError::FileNotFound;
                case FileError::ReadFailed:
                    return AppError::FileReadFailed;
            }
        })
        .and_then([](const std::string& content) {
            return parseNumber(content)
                .transform_error([](ParseError e) -> AppError {
                    // Convert ParseError to AppError
                    switch (e) {
                        case ParseError::Invalid:
                            return AppError::ParseInvalid;
                        case ParseError::Empty:
                            return AppError::ParseEmpty;
                    }
                });
        });
}
```

---

## Step 6: Testing Strategy

### Test both success and failure paths

```cpp
#include <gtest/gtest.h>

enum class ParseError { Empty, Invalid };
std::expected<int, ParseError> parse(const std::string& s);

class ParseTest : public ::testing::Test {
};

TEST_F(ParseTest, ParsesValidNumber) {
    auto result = parse("42");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 42);
}

TEST_F(ParseTest, RejectsEmpty) {
    auto result = parse("");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ParseError::Empty);
}

TEST_F(ParseTest, RejectsInvalid) {
    auto result = parse("abc");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ParseError::Invalid);
}

// Test composition
TEST_F(ParseTest, CompositionWorks) {
    auto result = parse("42")
        .transform([](int x) { return x * 2; });
    
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 84);
}

TEST_F(ParseTest, CompositionPropagatesError) {
    auto result = parse("")
        .transform([](int x) { return x * 2; });
    
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ParseError::Empty);
}
```

---

## Step 7: Performance Considerations

### Inline storage — no heap allocation

`std::expected` stores both `T` and `E` inline (on the stack or inside the containing object):

```cpp
// No heap allocation — T and E are stored together
std::expected<int, ParseError> result = parse(input);
// sizeof(result) == max(sizeof(int), sizeof(ParseError)) + 1 byte (discriminant)
```

**Compare to alternatives:**

```cpp
// std::variant — also no heap allocation (good)
std::variant<int, ParseError> var = /* ... */;

// std::pair with error code — two return channels (bad)
std::pair<bool, int> pair = /* ... */;  // error code separate

// Exception — heap allocation for unwinding (slow on error path)
try { /* ... */ } catch { /* ... */ }

// std::optional — can't carry error info
std::optional<int> opt = /* ... */;
```

### Cost summary

| Operation | Cost | Notes |
|-----------|------|-------|
| Create success | O(1) inline | Just copy/move the value |
| Create failure | O(1) inline | Just copy/move the error |
| Check has_value() | O(1) | Just check the discriminant |
| Access value | O(1) | Just dereference |
| Access error | O(1) | Just dereference |
| Compose (and_then) | O(1) | No allocation, just pass through |

**Compared to exceptions:**
- Exception on error: high cost (stack unwinding)
- Exception on success: zero cost (normal path)

**Compared to expected:**
- Expected on error: zero cost
- Expected on success: zero cost
Both paths are equally fast.

---

## Step 8: Migration Checklist

Use this to track your migration progress:

- [ ] Pick a small function to convert to `std::expected`
- [ ] Define an error enum for that function
- [ ] Rewrite the function to return `std::expected`
- [ ] Update all call sites to check the result
- [ ] Add tests for both success and failure paths
- [ ] Verify the code compiles and tests pass
- [ ] Document the error enum (what each case means)
- [ ] Pick the next function to convert
- [ ] Repeat until the critical path uses `std::expected`

---

## Quick Reference: Before/After

| Old Code | New Code | Benefit |
|----------|----------|---------|
| `throw` in errors | `return std::unexpected()` | No exceptions, `-fno-exceptions` compatible |
| `bool` + error code | `std::expected<T, E>` | Single return channel, type-safe errors |
| `std::optional<T>` | `std::expected<T, E>` | Carry error information, not just absence |
| `catch` blocks | `.and_then()` / `.or_else()` | Cleaner, composable error handling |
| Out-parameters | Return `std::expected` | Clearer signature, enforced error handling |
| Magic error numbers | Error enum | Self-documenting, compiler-checked |

---

## Common integration pitfalls

### Pitfall 1: Converting everything at once
**Don't:** rewrite your entire codebase in one go.
**Do:** start with one function, test it, then move to the next.

### Pitfall 2: Keeping exceptions and `std::expected` together
**Don't:** have some functions throw and others return `expected`.
**Do:** pick one error-handling strategy and stick to it in each module.

### Pitfall 3: Ignoring error types
**Don't:** use `std::string` for all errors.
**Do:** define proper error enums so errors are self-documenting.

### Pitfall 4: Not testing error paths
**Don't:** write tests only for the happy path.
**Do:** test every error case explicitly.

### Pitfall 5: Mixing `std::optional` and `std::expected`
**Don't:** use both in the same codebase without a clear rule.
**Do:** use `optional` for "value may not exist," `expected` for "operation succeeded or failed."

---

## Next steps

1. **Pick one real function** in your project that could use `std::expected`
2. **Follow steps 1-4** above to convert it
3. **Run your tests** to make sure nothing broke
4. **Document the error enum** (what each case means)
5. **Move to the next function**

You now have a clear path to adopting `std::expected` incrementally.
