# C++23 — Technical Learning Guide

This section is for learning the new features of **C++23** (the latest fully released standard, widely supported at the time this document was written). The goal is not just to show new syntax. For every feature we cover:

1. **Motivation** — why the committee added this feature, and what problem existed in older standards.
2. **Exact mechanism** — what happens at the compiler level, in overload resolution, in ABI, or in the standard library.
3. **Legacy code (pre-C++23)** — an equivalent implementation without the new feature, with its concrete downsides (boilerplate, runtime cost, possible undefined behavior, weak type safety).
4. **Modern code (C++23)** — the same problem solved with the new feature.
5. **Comparison** — performance, readability, type safety, and effect on the binary/ABI.

Every example is a **standalone, compilable `.cpp` file** (not a snippet inside Markdown), so the claims can be checked by actually running the code.

## Compiler requirements

| Feature | Minimum GCC | Minimum Clang |
|---|---|---|
| `std::expected` | GCC 12 (full support in 13) | Clang 16 |
| Deducing `this` | GCC 14 | Clang 18 |
| `std::print` / `std::println` | GCC 14 (some distros need `-lfmt`) | Clang 17+ with libc++ |
| Multidimensional `operator[]` | GCC 12 | Clang 12 |
| `std::mdspan` | GCC 15 | Clang 18 |
| `std::generator` (coroutines) | GCC 14 with `-fcoroutines` | Clang 17 |

> **Important technical note:** standard library support (libstdc++/libc++) lags behind the compiler's own language support. If your compiler accepts `-std=c++23`, that does **not** mean it ships `<expected>` or `<print>`. Compare your compiler version against the table above before running anything.

### Actual verification status in this development environment

All `legacy.cpp` examples, plus `01-std-expected/modern.cpp` and `02-deducing-this/modern.cpp`, were compiled and run on `g++ 13.3.0` and/or `clang++ 18.1.3` (both using `libstdc++`, no `libc++` installed) and **produced the expected output**. `build.py` automatically tries both compilers, because deducing-`this` is a core-language feature that `g++ 13` does not support but `clang++ 18` does.

The `03-std-print/modern.cpp`, `04-multidim-subscript-mdspan/modern.cpp`, and `05-std-generator/modern.cpp` examples **did not compile** in this environment, because `libstdc++ 13` does not yet ship the `<print>`, `<mdspan>`, and `<generator>` headers (those require libstdc++ 14/15), and `libc++` is not installed on this machine. These three examples were written strictly against the official cppreference.com documentation and the accepted text of the relevant proposal, and should compile unmodified on a toolchain that has these headers (for example GCC 14+, or Clang 18+ with `libc++`). Until you build with such a toolchain, this claim is *not* "verified by running it" — we state that honestly instead of presenting untested code as tested.

## Getting started

**New to this material?** Start here:

1. **Read [ROADMAP.md](ROADMAP.md)** — shows you the recommended learning order (8-12 hours total)
2. **Use [CHECKLIST.md](CHECKLIST.md)** — track your progress through each feature
3. **Then pick a feature** from the table below and follow the roadmap

---

## Running the examples

All build settings (compiler, `-std=` flags, which example is legacy vs. modern) are defined in this JSON file — **nothing is hardcoded** in the script or in the code:

```
docs/learning/cpp23-features/config/features.json
```

Run every example:

```bash
python3 docs/learning/cpp23-features/build.py
```

Run a single feature:

```bash
python3 docs/learning/cpp23-features/build.py 01-std-expected
```

## Feature list

| # | Feature | Proposal | Folder |
|---|---|---|---|
| 1 | `std::expected<T, E>` — error handling without exceptions | [P0323R12](01-std-expected/README.md) | [01-std-expected](01-std-expected/README.md) |
| 2 | Deducing `this` (explicit object parameters) | [P0847R7](02-deducing-this/README.md) | [02-deducing-this](02-deducing-this/README.md) |
| 3 | `std::print` / `std::println` | [P2093R14](03-std-print/README.md) | [03-std-print](03-std-print/README.md) |
| 4 | Multidimensional `operator[]` and `std::mdspan` | [P2128R6](04-multidim-subscript-mdspan/README.md) | [04-multidim-subscript-mdspan](04-multidim-subscript-mdspan/README.md) |
| 5 | `std::generator<T>` (lazy coroutine ranges) | [P2502R2](05-std-generator/README.md) | [05-std-generator](05-std-generator/README.md) |

Each folder has a `README.md` (full technical explanation) plus `examples/legacy.cpp` and `examples/modern.cpp`.
