# Phase 1: Error Handling — Complete Learning Package

Welcome to Phase 1 of the C++23 learning journey. This folder has everything you need to master `std::expected<T, E>`.

## Start here

1. **First time?** Read [README.md](README.md) — the "why" and "how" of `std::expected`
2. **Ready to learn?** Go through [TUTORIAL.md](TUTORIAL.md) step by step (5 sections, 2-3 hours)
3. **See real code?** Check [EXAMPLES.md](EXAMPLES.md) for 5 production-ready patterns
4. **Learning?** [MISTAKES.md](MISTAKES.md) shows common bugs and how to fix them
5. **Have existing code?** [INTEGRATION.md](INTEGRATION.md) guides adding `std::expected` to your project

## How to use these materials

### For learning (first time)
1. Read the README (30 min)
2. Work through the TUTORIAL step-by-step (2-3 hours)
3. Copy a pattern from EXAMPLES and try it yourself (1 hour)
4. Look at the modern.cpp code example and run it (30 min)

**Total time: 4-5 hours for a solid foundation**

### For reference (already familiar)
- **TUTORIAL** — sections 1-2 for quick memory refresh
- **EXAMPLES** — copy patterns directly into your code
- **MISTAKES** — check before committing your code
- **INTEGRATION** — strategy for refactoring existing code

### For your project
1. Read INTEGRATION.md (30 min)
2. Pick one function to convert (step 1 of integration guide)
3. Define your error enum
4. Follow the before/after examples
5. Test both success and failure paths
6. Move to the next function

## File descriptions

| File | Purpose | Time | Best for |
|------|---------|------|----------|
| [README.md](README.md) | Motivation, mechanism, comparison | 30 min | Understanding *why* |
| [TUTORIAL.md](TUTORIAL.md) | Step-by-step walkthrough, 5 sections | 2-3 hours | Learning *how* |
| [EXAMPLES.md](EXAMPLES.md) | 5 real-world patterns with full code | 1-2 hours | Seeing *when* to use it |
| [MISTAKES.md](MISTAKES.md) | 10 common bugs + fixes | 1 hour | Avoiding *pitfalls* |
| [INTEGRATION.md](INTEGRATION.md) | Incremental adoption strategy | 1 hour | Refactoring *existing* code |
| [legacy.cpp](examples/legacy.cpp) | Pre-C++23 error handling patterns | N/A | Understanding the problem |
| [modern.cpp](examples/modern.cpp) | C++23 `std::expected` solution | N/A | Seeing the solution |

## Learning outcomes

After completing this Phase 1 package, you will understand:

✓ What `std::expected<T, E>` is and why it exists
✓ How to create success and failure values
✓ How to check and access results
✓ How to compose operations with `.and_then()`, `.transform()`, `.or_else()`
✓ How to define proper error types (enums)
✓ Common pitfalls and how to avoid them
✓ How to incrementally add it to existing code
✓ How to test both success and failure paths

## Quick reference

### Create a result
```cpp
std::expected<int, ParseError> result = 42;  // success
std::expected<int, ParseError> result = std::unexpected(ParseError::Empty);  // failure
```

### Check and access
```cpp
if (result) {
    int value = *result;  // get success value
} else {
    auto error = result.error();  // get error
}
```

### Compose operations
```cpp
auto composed = step1(input)
    .and_then(step2)
    .and_then(step3)
    .transform([](int x) { return x * 2; });
```

### Convert error types
```cpp
auto converted = parseFile(path)
    .transform_error([](FileError e) -> AppError {
        return AppError::FileProblem;
    });
```

## Recommended learning path

```
Day 1 (2 hours):
  - Read README.md
  - Read TUTORIAL.md Sections 1-2
  - Run legacy.cpp and modern.cpp from examples

Day 2 (2 hours):
  - Read TUTORIAL.md Sections 3-4
  - Study EXAMPLES.md (pick 2 favorites)
  - Try modifying modern.cpp

Day 3 (1-2 hours):
  - Read MISTAKES.md and find the bugs
  - Read INTEGRATION.md
  - Start refactoring one real function in your project

Total: 5-6 hours for hands-on mastery
```

## Support

- **Confused about the concept?** Start with README.md and TUTORIAL.md Section 1
- **Can't get it to compile?** Check MISTAKES.md for common issues
- **Want to see it in action?** Look at EXAMPLES.md and run the code
- **Ready to use it?** Follow INTEGRATION.md for your real project
- **Stuck on a pattern?** Check EXAMPLES.md or MISTAKES.md for similar cases

## Next phase

After you complete Phase 1, you can:
- Move to **Phase 2: Deducing `this`** (cleaner generic code)
- Or jump to **Phase 3: `std::print`** (better I/O)
- See the main [ROADMAP.md](../ROADMAP.md) for the full learning path

Good luck, and feel free to revisit these materials as you code!
