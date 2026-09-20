# C++23 Learning Roadmap

This guide shows you the recommended order to learn the C++23 features in this section, and how they connect.

## Prerequisites (you should already know)

- Basic C++ (classes, templates, overloading)
- How to compile and run C++ programs
- What templates, lambdas, and function overloading are

## Learning path

### Phase 1: Error Handling (Foundation)
**Start here first** — most modern C++ code needs a robust way to handle errors.

1. **`std::expected<T, E>`** (1-2 hours)
   - Read `01-std-expected/README.md`
   - Compile and run both `legacy.cpp` and `modern.cpp`
   - Understand why exceptions and error codes are not enough
   - Play with `and_then()`, `or_else()`, `transform()` in the modern example

**What you'll get:** A type-safe, composable way to represent success or failure without exceptions. This pattern is used in Rust, and now available in modern C++.

---

### Phase 2: Writing Cleaner Generic Code (Intermediate)
**After you understand error handling**, learn how to write less boilerplate when writing generic/template code.

2. **Deducing `this`** (1-2 hours)
   - Read `02-deducing-this/README.md`
   - Compile and run both `legacy.cpp` and `modern.cpp`
   - Focus on why 4 overloads collapse into 1
   - Understand the CRTP example — this pattern is used in many libraries

**What you'll get:** Fewer overloads, simpler CRTP, and the ability to write recursive lambdas without `std::function`.

---

### Phase 3: Output and I/O (Practical)
**These are useful immediately in any program.**

3. **`std::print` / `std::println`** (30 minutes)
   - Read `03-std-print/README.md`
   - Compile and run `legacy.cpp` (should work on any compiler)
   - Understand why `printf` and `iostream` both have problems
   - Note: modern.cpp may not compile yet (needs a newer standard library)

**What you'll get:** Cleaner, type-safe, and faster formatted output. No more stateful manipulators.

---

### Phase 4: Working with Multidimensional Data (Advanced)
**Use this if you work with matrices, images, sensor buffers, or other 2D/3D data.**

4. **Multidimensional `operator[]` and `std::mdspan`** (2-3 hours)
   - Read `04-multidim-subscript-mdspan/README.md`
   - Compile and run `legacy.cpp` (should work)
   - Understand why `vector<vector<T>>` is bad for cache performance
   - Understand why manual flat-index math is error-prone
   - Note: modern.cpp needs a very new standard library

**What you'll get:** Type-safe, cache-friendly matrix/tensor operations without allocation overhead.

---

### Phase 5: Lazy Evaluation and Sequences (Expert)
**Use this if you need to generate large sequences or work with infinite ranges.**

5. **`std::generator<T>`** (2-3 hours)
   - Read `05-std-generator/README.md`
   - Compile and run `legacy.cpp`
   - Understand coroutines (C++20) and `co_yield`
   - Understand why manual iterators are hard to write correctly
   - Note: modern.cpp needs a very new standard library

**What you'll get:** Clean, lazy generation of sequences. Composes with `std::views` naturally. No manual state management.

---

## Estimated total time

- **Phase 1 + Phase 2:** 3-4 hours (foundation)
- **Phase 3:** 30 minutes (practical)
- **Phase 4 + 5:** 4-6 hours (advanced, optional depending on your domain)

**Total:** 8-12 hours for a solid understanding of all five features.

## How to use this roadmap

1. Start with Phase 1. Do not skip it.
2. After Phase 1, you can jump to Phase 3 or Phase 2 depending on your needs.
3. Phases 4 and 5 are independent of each other — choose based on your work domain.
4. For each feature:
   - Read the README for context and motivation
   - Run `build.py` to compile both legacy and modern examples
   - Read both `.cpp` files side-by-side
   - Try modifying them to understand what breaks

## Common questions

**Q: Do I need to learn all five?**
A: No. At minimum, learn `std::expected` (Phase 1). The others are useful depending on what you code. Deducing-`this` helps if you write templates or libraries.

**Q: Can I skip the legacy code?**
A: No. The legacy code shows **why** the modern feature exists. Without it, the modern feature just looks like syntax.

**Q: What if my compiler doesn't support some features?**
A: `01-std-expected` and `02-deducing-this` should work on any recent compiler (GCC 13+, Clang 17+). The others may need a very new standard library. See the main README for details.

**Q: How do I practice?**
A: After reading each feature, try rewriting one of your own programs to use it. If you code a lot of error handling, `std::expected` is a great starting point.
