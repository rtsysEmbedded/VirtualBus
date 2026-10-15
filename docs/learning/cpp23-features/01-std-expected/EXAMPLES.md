# Real-World Examples — `std::expected` in Practice

Copy these patterns directly into your projects. Each example solves a real problem.

---

## Example 1: Configuration File Parser

**Problem:** parse a config file line-by-line, report errors with line numbers.

```cpp
#include <expected>
#include <fstream>
#include <string>
#include <vector>

enum class ConfigError {
    FileNotFound,
    InvalidLine,
    MissingValue,
    NotANumber
};

struct ConfigError {
    ConfigError kind;
    int lineNumber;
    std::string context;
};

struct ConfigValue {
    std::string key;
    int value;
};

std::expected<ConfigValue, ConfigError> parseLine(const std::string& line, int lineNum) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') {
        return std::unexpected(ConfigError{
            .kind = ConfigError::InvalidLine,
            .lineNumber = lineNum,
            .context = "empty or comment"
        });
    }
    
    size_t eqPos = line.find('=');
    if (eqPos == std::string::npos) {
        return std::unexpected(ConfigError{
            .kind = ConfigError::MissingValue,
            .lineNumber = lineNum,
            .context = line
        });
    }
    
    std::string key = line.substr(0, eqPos);
    std::string valueStr = line.substr(eqPos + 1);
    
    try {
        int value = std::stoi(valueStr);
        return ConfigValue{key, value};
    } catch (...) {
        return std::unexpected(ConfigError{
            .kind = ConfigError::NotANumber,
            .lineNumber = lineNum,
            .context = valueStr
        });
    }
}

struct ParseResult {
    std::vector<ConfigValue> values;
    std::vector<ConfigError> errors;
};

ParseResult loadConfig(const std::string& filename) {
    ParseResult result;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        result.errors.push_back(ConfigError{
            .kind = ConfigError::FileNotFound,
            .lineNumber = 0,
            .context = filename
        });
        return result;
    }
    
    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        auto parsed = parseLine(line, lineNum);
        if (parsed) {
            result.values.push_back(*parsed);
        } else {
            result.errors.push_back(parsed.error());
        }
    }
    
    return result;
}

// Usage:
int main() {
    auto result = loadConfig("app.conf");
    
    std::cout << "Loaded " << result.values.size() << " values\n";
    
    for (const auto& cfg : result.values) {
        std::cout << cfg.key << " = " << cfg.value << '\n';
    }
    
    for (const auto& err : result.errors) {
        std::cout << "Line " << err.lineNumber << ": error\n";
    }
    
    return 0;
}
```

---

## Example 2: Network Request (Simulate MQTT Connection)

**Problem:** connect to a server, handle various failure modes.

```cpp
#include <expected>
#include <chrono>
#include <string>

enum class ConnectionError {
    TimeoutError,
    HostNotFound,
    PermissionDenied,
    ConnectionRefused,
    UnknownError
};

class MQTTConnection {
public:
    std::expected<void, ConnectionError> connect(
        const std::string& host,
        int port,
        std::chrono::seconds timeout) {
        
        // Simulate network checks
        if (host.empty()) {
            return std::unexpected(ConnectionError::HostNotFound);
        }
        
        if (port < 1 || port > 65535) {
            return std::unexpected(ConnectionError::PermissionDenied);
        }
        
        // Simulate connection timeout
        if (timeout.count() < 1) {
            return std::unexpected(ConnectionError::TimeoutError);
        }
        
        // Success — return void (use std::expected<void, E>)
        return {};
    }
    
    std::expected<std::string, ConnectionError> subscribe(const std::string& topic) {
        if (!isConnected_) {
            return std::unexpected(ConnectionError::ConnectionRefused);
        }
        
        if (topic.empty()) {
            return std::unexpected(ConnectionError::UnknownError);
        }
        
        return "subscribed to " + topic;
    }
    
private:
    bool isConnected_ = false;
};

// Usage with composition:
int main() {
    MQTTConnection conn;
    
    auto result = conn.connect("mqtt.example.com", 1883, std::chrono::seconds(5))
        .and_then([&conn](void) {
            return conn.subscribe("sensors/temperature");
        });
    
    if (result) {
        std::cout << *result << '\n';  // prints subscription message
    } else {
        std::cout << "Connection failed\n";
    }
    
    return 0;
}
```

Note: `std::expected<void, E>` is used when you want to return either "success with no value" or an error.

---

## Example 3: JSON Parser (Simplified)

**Problem:** parse nested JSON, report the exact error location.

```cpp
#include <expected>
#include <string>
#include <map>
#include <variant>

enum class JsonError {
    InvalidSyntax,
    UnexpectedEndOfInput,
    TypeMismatch
};

struct JsonValue {
    std::variant<std::string, int, std::map<std::string, JsonValue>> data;
};

class JsonParser {
public:
    std::expected<JsonValue, JsonError> parse(const std::string& json) {
        pos_ = 0;
        input_ = json;
        skipWhitespace();
        return parseValue();
    }
    
private:
    std::string input_;
    size_t pos_ = 0;
    
    void skipWhitespace() {
        while (pos_ < input_.size() && std::isspace(input_[pos_])) {
            pos_++;
        }
    }
    
    std::expected<JsonValue, JsonError> parseValue() {
        skipWhitespace();
        
        if (pos_ >= input_.size()) {
            return std::unexpected(JsonError::UnexpectedEndOfInput);
        }
        
        char c = input_[pos_];
        
        if (c == '"') {
            return parseString()
                .transform([](std::string s) { return JsonValue{s}; });
        } else if (std::isdigit(c)) {
            return parseNumber()
                .transform([](int n) { return JsonValue{n}; });
        } else if (c == '{') {
            return parseObject()
                .transform([](auto obj) { return JsonValue{obj}; });
        } else {
            return std::unexpected(JsonError::InvalidSyntax);
        }
    }
    
    std::expected<std::string, JsonError> parseString() {
        if (input_[pos_] != '"') {
            return std::unexpected(JsonError::InvalidSyntax);
        }
        
        pos_++;
        std::string result;
        
        while (pos_ < input_.size() && input_[pos_] != '"') {
            result += input_[pos_];
            pos_++;
        }
        
        if (pos_ >= input_.size()) {
            return std::unexpected(JsonError::UnexpectedEndOfInput);
        }
        
        pos_++;  // skip closing quote
        return result;
    }
    
    std::expected<int, JsonError> parseNumber() {
        size_t start = pos_;
        while (pos_ < input_.size() && std::isdigit(input_[pos_])) {
            pos_++;
        }
        
        try {
            return std::stoi(input_.substr(start, pos_ - start));
        } catch (...) {
            return std::unexpected(JsonError::InvalidSyntax);
        }
    }
    
    std::expected<std::map<std::string, JsonValue>, JsonError> parseObject() {
        std::map<std::string, JsonValue> obj;
        
        if (input_[pos_] != '{') {
            return std::unexpected(JsonError::InvalidSyntax);
        }
        
        pos_++;
        skipWhitespace();
        
        if (input_[pos_] == '}') {
            pos_++;
            return obj;
        }
        
        // Simplified: just parse key-value pairs
        while (pos_ < input_.size()) {
            auto key = parseString();
            if (!key) return std::unexpected(key.error());
            
            skipWhitespace();
            if (input_[pos_] != ':') {
                return std::unexpected(JsonError::InvalidSyntax);
            }
            pos_++;
            
            auto value = parseValue();
            if (!value) return std::unexpected(value.error());
            
            obj[*key] = *value;
            
            skipWhitespace();
            if (input_[pos_] == '}') break;
            if (input_[pos_] == ',') pos_++;
        }
        
        pos_++;  // skip }
        return obj;
    }
};

// Usage:
int main() {
    JsonParser parser;
    auto result = parser.parse(R"({"name":"sensor","value":42})");
    
    if (result) {
        std::cout << "Parse succeeded\n";
    } else {
        std::cout << "Parse failed\n";
    }
    
    return 0;
}
```

---

## Example 4: Validation Chain

**Problem:** validate user input through multiple steps.

```cpp
#include <expected>
#include <string>
#include <cctype>

enum class ValidationError {
    TooShort,
    TooLong,
    InvalidCharacters,
    NoDigits,
    NoUppercase
};

std::expected<std::string, ValidationError> validateUsername(const std::string& name) {
    return validateLength(name, 3, 20)
        .and_then([](auto n) { return validateNoSpecialChars(n); })
        .and_then([](auto n) { return validateNoSpaces(n); });
}

std::expected<std::string, ValidationError> validateLength(
    const std::string& name, int minLen, int maxLen) {
    
    if (name.size() < minLen) {
        return std::unexpected(ValidationError::TooShort);
    }
    if (name.size() > maxLen) {
        return std::unexpected(ValidationError::TooLong);
    }
    return name;
}

std::expected<std::string, ValidationError> validateNoSpecialChars(const std::string& name) {
    for (char c : name) {
        if (!std::isalnum(c) && c != '_') {
            return std::unexpected(ValidationError::InvalidCharacters);
        }
    }
    return name;
}

std::expected<std::string, ValidationError> validateNoSpaces(const std::string& name) {
    if (name.find(' ') != std::string::npos) {
        return std::unexpected(ValidationError::InvalidCharacters);
    }
    return name;
}

std::expected<std::string, ValidationError> validatePassword(const std::string& pwd) {
    return validateLength(pwd, 8, 128)
        .and_then(validateHasDigit)
        .and_then(validateHasUppercase);
}

std::expected<std::string, ValidationError> validateHasDigit(const std::string& pwd) {
    for (char c : pwd) {
        if (std::isdigit(c)) return pwd;
    }
    return std::unexpected(ValidationError::NoDigits);
}

std::expected<std::string, ValidationError> validateHasUppercase(const std::string& pwd) {
    for (char c : pwd) {
        if (std::isupper(c)) return pwd;
    }
    return std::unexpected(ValidationError::NoUppercase);
}

// Usage:
int main() {
    auto userResult = validateUsername("john_doe");
    auto pwdResult = validatePassword("MyPassword123");
    
    if (userResult && pwdResult) {
        std::cout << "Validation passed\n";
    } else {
        if (!userResult) {
            std::cout << "Username validation failed\n";
        }
        if (!pwdResult) {
            std::cout << "Password validation failed\n";
        }
    }
    
    return 0;
}
```

---

## Example 5: Database Query (Simulated)

**Problem:** query results with multiple failure modes.

```cpp
#include <expected>
#include <vector>
#include <string>

enum class DatabaseError {
    ConnectionLost,
    InvalidQuery,
    NotFound,
    DuplicateKey,
    UnknownError
};

struct Record {
    int id;
    std::string name;
    std::string email;
};

class Database {
public:
    std::expected<Record, DatabaseError> findById(int id) {
        if (!isConnected_) {
            return std::unexpected(DatabaseError::ConnectionLost);
        }
        
        if (id < 0) {
            return std::unexpected(DatabaseError::InvalidQuery);
        }
        
        // Simulated lookup
        auto it = records_.find(id);
        if (it == records_.end()) {
            return std::unexpected(DatabaseError::NotFound);
        }
        
        return it->second;
    }
    
    std::expected<void, DatabaseError> insert(const Record& r) {
        if (!isConnected_) {
            return std::unexpected(DatabaseError::ConnectionLost);
        }
        
        if (records_.count(r.id)) {
            return std::unexpected(DatabaseError::DuplicateKey);
        }
        
        records_[r.id] = r;
        return {};
    }
    
    void connect() { isConnected_ = true; }
    void disconnect() { isConnected_ = false; }
    
private:
    std::map<int, Record> records_;
    bool isConnected_ = false;
};

// Usage with error handling:
int main() {
    Database db;
    db.connect();
    
    // Chain operations: insert, then find, then update
    auto result = db.insert(Record{1, "Alice", "alice@example.com"})
        .and_then([&db](void) {
            return db.findById(1)
                .transform([](Record r) { return r; });
        });
    
    if (result) {
        std::cout << "Found: " << result->name << '\n';
    } else {
        std::cout << "Operation failed\n";
    }
    
    return 0;
}
```

---

## When to use each pattern

| Problem | Pattern | Example |
|---------|---------|---------|
| Parsing (files, config, JSON) | Enum errors + `and_then` | Config parser (Example 1) |
| Network/I/O | Enum errors + `or_else` recovery | MQTT connection (Example 2) |
| Validation chains | Validate-then-pass pattern | Password validator (Example 5) |
| Bulk operations | Collect successes + failures | Config file with errors |
| Complex workflows | Composition with `transform` | JSON parser (Example 3) |

Use the checklist in the main Phase 1 materials to practice each pattern.
