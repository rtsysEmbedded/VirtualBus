# `std::expected<T, E>` — Error Handling Without Exceptions or Out-Parameters

**Proposal:** [P0323R12](https://wg21.link/P0323R12) — accepted in C++23, header `<expected>`.

## 1. The problem in older standards

Before C++23, returning "either a valid value or an error" from a function usually meant one of these:

1. **Throwing an exception** — noticeable runtime cost (stack unwinding), unusable in real-time/embedded code where exceptions are disabled (`-fno-exceptions`), and the error path is invisible in the function signature.
2. **Return code + out-parameter (C style)**: `bool parse(const std::string&, int& out)`. Problem: the caller forgetting to check the return value is a common, silent bug. The compiler does not enforce anything.
3. **`std::pair<bool, T>` or `std::optional<T>`** — `optional` only says "there is no value," but it does **not** carry the **reason** why the value is missing.
4. **`std::variant<T, ErrorCode>`** — conceptually similar to `expected`, but its API (`std::visit`, `std::get_if`) was not designed for this exact pattern, and it has no monadic composition (`and_then`, `or_else`).

Result: in embedded/real-time systems (exactly the domain this repository — VirtualBus — lives in, where CANopen/MQTT deal with predictable failure modes), raw numeric error codes are commonly used. They have zero type safety and are easy to ignore.

## 2. How `std::expected` works

`std::expected<T, E>` is a union-like type (similar to `variant`, but limited to exactly two states) that holds either:

- a value of type `T` on success (`has_value() == true`), or
- an error value of type `E`, constructed with `std::unexpected<E>`.

Key technical points:

- **No heap allocation**: unlike `std::variant`, the `expected` implementation guarantees that `T` and `E` are stored inline (on the stack/inside the object); there is no extra dynamic allocation. This matters a lot for embedded code.
- **Monadic operations**: `and_then`, `transform`, `or_else`, and `transform_error` let you chain fallible operations without nested `if` statements — the same pattern as `Result<T, E>` in Rust or `Either` in Haskell.
- **Accessing the error**: `error()` is only well-defined when `has_value() == false`; otherwise it is undefined behavior (usually an assert in debug builds).
- **No exception overhead**: it can be compiled fully with `-fno-exceptions`, because there is no throw/catch anywhere in `expected`'s internals.
- **Visible in the function signature**: `std::expected<Config, ParseError> parseConfig(...)` explicitly says the function can fail, and the compiler forces the caller to interact with the return value (ignoring it produces a warning, because `expected` is marked `[[nodiscard]]`).

## 3. Comparing the examples

`examples/legacy.cpp`: parses a numeric string using pre-C++23 approaches (manual error code + C-style `errno`, and an exception-based version) — and shows why forgetting to check the error compiles cleanly with no warning at all.

`examples/modern.cpp`: the same logic with `std::expected`, including an `and_then` chain that composes several fallible steps, and shows that ignoring the `[[nodiscard]]` return value produces a compiler warning.

## 4. Technical summary

| Criterion | Old approach (error code / out-param) | Old approach (exceptions) | `std::expected` |
|---|---|---|---|
| Cost on the success path | zero | zero | zero (inline storage) |
| Cost on the error path | zero | high (stack unwinding) | zero |
| Compiler-enforced error check | no | no (until a `catch`) | yes (`[[nodiscard]]`) |
| Usable without RTTI/exceptions | yes | no | yes |
| Composability | weak | medium (nested try/catch) | strong (`and_then`/`or_else`) |
| Error info visible in signature | no (only via a comment) | no | yes (explicit `E` type) |
