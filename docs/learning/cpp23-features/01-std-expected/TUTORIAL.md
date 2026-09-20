# `std::expected` — Step-by-Step Tutorial

This tutorial walks you through `std::expected` from basics to practical patterns. Work through each section in order.

## Section 1: The Basics (30 minutes)

### What is `std::expected`?

`std::expected<T, E>` holds **either** a value of type `T` **or** an error of type `E`. Think of it as:

```cpp
std::expected<int, std::string> result = parse("42");

if (result) {  // Did we get a value?
    int value = *result;  // Access the value with *
    std::cout << "Got: " << value << '\n';
} else {
    std::string error = result.error();  // Access the error
    std::cout << "Error: " << error << '\n';
}
```

### Creating success and failure

**Success** — the normal `T` value:
```cpp
std::expected<int, std::string> success = 42;
// or explicitly:
std::expected<int, std::string> success = std::expected<int, std::string>(42);
```

**Failure** — wrap the error with `std::unexpected`:
```cpp
std::expected<int, std::string> failure = std::unexpected("not a number");
// or with a return statement:
std::expected<int, std::string> parse(const std::string& s) {
    if (s.empty()) {
        return std::unexpected("input is empty");
    }
    return 42;
}
```

### Checking for success/failure

```cpp
std::expected<int, std::string> result = parse("42");

// Method 1: boolean check
if (result) {
    // success path
} else {
    // failure path
}

// Method 2: explicit
if (result.has_value()) {
    // success path
}

// Method 3: checking for error
if (!result) {
    // failure path — result.error() is available
}
```

### Accessing the value or error

```cpp
std::expected<int, std::string> result = parse("42");

// Get the value (only safe if has_value() is true)
int value = *result;
int value2 = result.value();  // throws std::bad_expected_access if has_value() == false

// Get the error (only safe if has_value() == false)
std::string error = result.error();

// Provide a default if failure
int valueOrDefault = result.value_or(0);  // returns 0 if has_value() == false
```

---

## Section 2: Error Types (30 minutes)

### Using an enum for errors

This is the most common pattern:

```cpp
enum class ParseError {
    EmptyInput,
    NotANumber,
    OutOfRange
};

std::expected<int, ParseError> parse(const std::string& s) {
    if (s.empty()) {
        return std::unexpected(ParseError::EmptyInput);
    }
    // ... more checks ...
    if (/* some error */) {
        return std::unexpected(ParseError::OutOfRange);
    }
    return 42;
}

// Caller can check the specific error:
auto result = parse("");
if (!result) {
    if (result.error() == ParseError::EmptyInput) {
        std::cout << "Input was empty\n";
    } else if (result.error() == ParseError::OutOfRange) {
        std::cout << "Number too large\n";
    }
}
```

### Using a struct for richer errors

```cpp
struct ParseError {
    std::string message;
    int lineNumber;
    int columnNumber;
};

std::expected<Config, ParseError> parseConfig(const std::string& text) {
    // ...
    return std::unexpected(ParseError{
        .message = "expected '='",
        .lineNumber = 5,
        .columnNumber = 42
    });
}

// Caller gets full context:
auto result = parseConfig(configText);
if (!result) {
    std::cerr << "Parse error at " << result.error().lineNumber
              << ":" << result.error().columnNumber
              << " - " << result.error().message << '\n';
}
```

### Using `std::string` for simple cases

```cpp
std::expected<int, std::string> parse(const std::string& s) {
    if (s.empty()) {
        return std::unexpected("input is empty");
    }
    if (!std::isdigit(s[0])) {
        return std::unexpected("not a number");
    }
    return std::stoi(s);
}
```

**When to use each:**
- **Enum** — when you have a small, fixed set of error cases (recommended)
- **Struct** — when you need rich context (line/column, suggestions, etc.)
- **String** — only for prototyping or when errors are truly ad-hoc

---

## Section 3: Composing Operations (1 hour)

The power of `std::expected` is **composing** multiple fallible operations.

### Without composition (nested if-else)

```cpp
std::expected<int, ParseError> pipeline(const std::string& input) {
    auto step1 = parseNumber(input);
    if (!step1) {
        return std::unexpected(step1.error());
    }
    
    auto step2 = validate(*step1);
    if (!step2) {
        return std::unexpected(step2.error());
    }
    
    auto step3 = transform(*step2);
    if (!step3) {
        return std::unexpected(step3.error());
    }
    
    return *step3;
}
```

This is readable but repetitive.

### With composition: `and_then`

`and_then` chains operations. If any step fails, it short-circuits and returns that failure:

```cpp
std::expected<int, ParseError> pipeline(const std::string& input) {
    return parseNumber(input)
        .and_then(validate)
        .and_then(transform);
}

// The three functions:
std::expected<int, ParseError> parseNumber(const std::string& s);
std::expected<int, ParseError> validate(int n);
std::expected<int, ParseError> transform(int n);
```

Much cleaner! If `parseNumber` fails, `and_then(validate)` is not called; the error propagates.

### With composition: `transform`

`transform` is like `and_then`, but for functions that return a plain value (not `std::expected`):

```cpp
std::expected<int, ParseError> pipeline(const std::string& input) {
    return parseNumber(input)
        .transform([](int n) { return n * 2; })
        .transform([](int n) { return n + 10; })
        .and_then(validate);
}

// The transform lambdas return int (not std::expected<int, ParseError>)
```

### With composition: `or_else`

`or_else` provides a recovery path. If the previous step failed, `or_else` runs:

```cpp
std::expected<int, ParseError> parseWithDefault(const std::string& input) {
    return parseNumber(input)
        .or_else([](ParseError err) -> std::expected<int, ParseError> {
            if (err == ParseError::EmptyInput) {
                return 0;  // default for empty input
            }
            return std::unexpected(err);  // re-throw other errors
        });
}
```

### With composition: `transform_error`

`transform_error` converts one error type to another:

```cpp
enum class AppError { InvalidConfig, NetworkFailed };

std::expected<Config, AppError> loadConfig(const std::string& path) {
    return parseConfigFile(path)
        .transform_error([](ParseError pe) -> AppError {
            return AppError::InvalidConfig;  // convert to app error
        });
}

// ParseError is converted to AppError automatically
```

### Real example: parsing a config line

```cpp
enum class ParseError { EmptyInput, MissingEqual, NotANumber };

struct ConfigLine { std::string key; int value; };

std::expected<ConfigLine, ParseError> parseLine(const std::string& line) {
    return parseKey(line)
        .and_then([&line](std::string key) {
            return parseValue(line).transform([key](int val) {
                return ConfigLine{key, val};
            });
        });
}

std::expected<std::string, ParseError> parseKey(const std::string& line) {
    auto eqPos = line.find('=');
    if (eqPos == std::string::npos) {
        return std::unexpected(ParseError::MissingEqual);
    }
    return line.substr(0, eqPos);
}

std::expected<int, ParseError> parseValue(const std::string& line) {
    auto eqPos = line.find('=');
    if (eqPos == std::string::npos) {
        return std::unexpected(ParseError::MissingEqual);
    }
    try {
        return std::stoi(line.substr(eqPos + 1));
    } catch (...) {
        return std::unexpected(ParseError::NotANumber);
    }
}
```

---

## Section 4: Practical Patterns (1 hour)

### Pattern: Bulk error handling

Parse multiple inputs, collect all errors:

```cpp
std::vector<std::string> inputs = {"10", "abc", "20", ""};
std::vector<int> successes;
std::vector<std::pair<std::string, ParseError>> failures;

for (const auto& input : inputs) {
    auto result = parse(input);
    if (result) {
        successes.push_back(*result);
    } else {
        failures.push_back({input, result.error()});
    }
}

// Now handle all successes and all failures together
std::cout << "Parsed: " << successes.size() << ", Errors: " << failures.size() << '\n';
```

### Pattern: Forwarding errors up the stack

Let errors propagate automatically with lambdas:

```cpp
std::expected<Config, ParseError> loadAllConfigs(const std::vector<std::string>& paths) {
    Config merged;
    for (const auto& path : paths) {
        auto cfg = loadConfig(path)
            .transform([&merged](Config c) {
                merged.merge(c);
                return unit();
            });
        
        if (!cfg) {
            return std::unexpected(cfg.error());  // stop on first error
        }
    }
    return merged;
}
```

### Pattern: With optional fallback

Combine `std::expected` and `std::optional`:

```cpp
std::expected<std::string, ParseError> getConfigValue(const std::string& key) {
    auto cfg = loadConfig();
    if (!cfg) {
        return std::unexpected(cfg.error());
    }
    
    // Look for key, return error if not found
    auto value = cfg->getValue(key);  // optional<string>
    if (!value) {
        return std::unexpected(ParseError::KeyNotFound);
    }
    return *value;
}
```

### Pattern: Delayed error handling

Sometimes you want to collect successes and defer error handling:

```cpp
struct Result {
    std::vector<int> values;
    std::vector<ParseError> errors;
};

Result parseMany(const std::vector<std::string>& inputs) {
    Result r;
    for (const auto& input : inputs) {
        auto val = parse(input);
        if (val) {
            r.values.push_back(*val);
        } else {
            r.errors.push_back(val.error());
        }
    }
    return r;
}
```

---

## Section 5: Working with Existing Code (30 minutes)

### Converting from exceptions

Old code:
```cpp
int parseOld(const std::string& s) {
    if (s.empty()) throw std::runtime_error("empty");
    return std::stoi(s);  // throws if invalid
}
```

New code:
```cpp
std::expected<int, std::string> parseNew(const std::string& s) {
    if (s.empty()) {
        return std::unexpected("empty");
    }
    try {
        return std::stoi(s);
    } catch (const std::exception& e) {
        return std::unexpected(e.what());
    }
}
```

### Converting from error codes

Old code:
```cpp
bool parseOld(const std::string& s, int& out, int& errCode) {
    if (s.empty()) { errCode = 1; return false; }
    try {
        out = std::stoi(s);
        return true;
    } catch (...) {
        errCode = 2;
        return false;
    }
}
```

New code:
```cpp
enum class ParseError { EmptyInput, InvalidNumber };

std::expected<int, ParseError> parseNew(const std::string& s) {
    if (s.empty()) {
        return std::unexpected(ParseError::EmptyInput);
    }
    try {
        return std::stoi(s);
    } catch (...) {
        return std::unexpected(ParseError::InvalidNumber);
    }
}
```

---

## Practice exercises

1. **Write a URL parser** that returns `std::expected<URL, ParseError>` with at least 3 error cases
2. **Compose three functions** using `and_then`: parse → validate → transform
3. **Create an enum error type** with 5+ cases and use `transform_error` to convert it
4. **Parse a CSV file** and collect both successes and failures separately
5. **Refactor existing code** in your project to use `std::expected` instead of exceptions or out-parameters

Next: read the [EXAMPLES.md](EXAMPLES.md) for real-world code samples.
