# `std::print` / `std::println`

**Proposal:** [P2093R14](https://wg21.link/P2093R14) — accepted in C++23, header `<print>` (built on top of `<format>`, which arrived in C++20).

## 1. The problem in older standards

Text output in C++ has a difficult history:

1. **`printf` (C style)**: fast and compact, but **not type-safe** — the argument type is never statically checked against the format specifier (`%d`, `%s`, ...). A mismatch causes undefined behavior (one of the most common sources of security bugs in the whole history of C/C++).
2. **`std::cout` (iostream)**: type-safe, but:
   - The chained `<<` syntax is very verbose for anything beyond trivial formatting (padding, decimal precision, numeric base require `std::setw`, `std::setprecision`, `std::hex`, ...).
   - **Stateful manipulators**: once you set `std::hex`, it stays set on the stream and can silently affect later, unrelated output — a common source of invisible bugs.
   - Overhead (locale synchronization with `stdio`, harder to inline) makes it noticeably slower in I/O-heavy programs unless you call `std::ios::sync_with_stdio(false)`.
3. **`std::format` (C++20)**: fixed type safety and syntax (`std::format("{}", x)`), but it only **returns a string**; you still need `std::cout <<` or `std::fputs` to actually print it — an unnecessary allocation + copy for every log message.

## 2. How `std::print` works

`std::print(fmt, args...)` writes directly to a `FILE*` (`stdout` by default), without building an intermediate `std::string`:

- It uses the same compile-time engine as `std::format`: the format string is parsed and validated **at compile time** (if the number of `{}` placeholders doesn't match the number of arguments, you get a compile error — not runtime undefined behavior like `printf`).
- `std::println` is `std::print` plus an automatic trailing `'\n'` — an implementation can emit it as a single, more atomic write (reducing interleaved output in multi-threaded programs compared to two separate calls).
- Unlike `iostream`, it is **stateless**: there is no manipulator state that lingers on the output stream.
- On implementations that detect a Unicode-aware terminal (e.g. MSVC on Windows), `std::print` can correctly render UTF-8 output in the console, where classic `printf`/`cout` often have encoding problems on Windows.

## 3. Comparing the examples

`examples/legacy.cpp`: the same logging output with `printf` (showing the danger of a type mismatch that still compiles) and with `iostream` (showing the verbosity and the stateful-manipulator problem).

`examples/modern.cpp`: the same output with `std::print`/`std::println` — no stateful manipulators, with compile-time validation.

## 4. Technical summary

| Criterion | `printf` | `iostream` | `std::format` (C++20) | `std::print` (C++23) |
|---|---|---|---|---|
| Type safety | no (runtime UB) | yes | yes (compile-time) | yes (compile-time) |
| Needs an intermediate allocation | no | no | yes (`std::string`) | no (writes straight to the file) |
| Stateful manipulators | none | yes (dangerous) | none | none |
| Readability of complex formatting | low | lower | high | high |
