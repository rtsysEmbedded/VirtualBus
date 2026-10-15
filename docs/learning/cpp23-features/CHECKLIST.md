# Learning Checklist

Use this checklist to track your progress through the C++23 learning material. Copy it, check off items as you complete them, and use it to stay organized.

## Phase 1: Error Handling

### `std::expected<T, E>` (01-std-expected)

- [ ] Read the README
- [ ] Understand the motivation — why exceptions and error codes are not enough
- [ ] Compile and run `legacy.cpp`
- [ ] Read `legacy.cpp` and understand what each pattern does
- [ ] Compile and run `modern.cpp`
- [ ] Read `modern.cpp` and identify the `and_then()`, `or_else()`, and `transform()` calls
- [ ] Understand `[[nodiscard]]` — why it prevents silent ignoring of the return value
- [ ] Try modifying `modern.cpp` to add a new error case (hint: add to `ParseError` enum and check for it)
- [ ] Write one small example of your own using `std::expected` (e.g., a function that parses a config line)

---

## Phase 2: Writing Generic Code

### Deducing `this` (02-deducing-this)

- [ ] Read the README
- [ ] Understand the 3 problems: (1) four overloads, (2) CRTP boilerplate, (3) recursive lambda
- [ ] Compile and run `legacy.cpp` on g++
- [ ] Compile and run `modern.cpp` on clang++ (if g++ fails)
- [ ] Read both `.cpp` files side-by-side and count the overloads/template boilerplate in legacy vs. modern
- [ ] Understand the `CRTP` example — why the base class is not templated in the modern version
- [ ] Understand the recursive lambda — what `this auto&& self` does
- [ ] Try adding a method to `Widget` that multiplies the value by 2 (must work in both legacy and modern)
- [ ] Try the CRTP pattern with your own derived class

---

## Phase 3: Output and I/O

### `std::print` / `std::println` (03-std-print)

- [ ] Read the README
- [ ] Understand the three old approaches and their problems: `printf` (not type-safe), `iostream` (verbose, stateful), `std::format` (needs a string)
- [ ] Compile and run `legacy.cpp`
- [ ] Read `legacy.cpp` and find the line where `%d` should be `%f`
- [ ] Note the stateful manipulator bug: why does the second integer print in hex?
- [ ] Try to compile `modern.cpp` (may fail if stdlib is too old)
- [ ] If `modern.cpp` compiled: read it and understand why there is no stateful manipulator issue
- [ ] Understand the format string syntax: `{:.2f}` means "float with 2 decimal places"
- [ ] Try rewriting a `printf` or `cout` line from your own code as `std::print` (when your compiler supports it)

---

## Phase 4: Multidimensional Data

### Multidimensional `operator[]` and `std::mdspan` (04-multidim-subscript-mdspan)

- [ ] Read the README
- [ ] Understand why `vector<vector<T>>` causes cache misses (memory not contiguous)
- [ ] Understand why manual flat-index math (`row * cols + col`) is easy to get wrong
- [ ] Compile and run `legacy.cpp`
- [ ] Read `legacy.cpp`: which pattern does each class use? What are the downsides?
- [ ] Try to compile `modern.cpp` (may fail if stdlib is too old — that is OK)
- [ ] If `modern.cpp` compiled: run it and compare the output
- [ ] Understand `extent(0)` and `extent(1)` — how they replace hard-coded dimensions
- [ ] If you work with matrices/images: sketch how you would rewrite your code with `mdspan`

---

## Phase 5: Lazy Sequences

### `std::generator<T>` (05-std-generator)

- [ ] Read the README
- [ ] Understand why eager (building the whole vector first) is bad for large sequences
- [ ] Understand why manual iterators require boilerplate and are error-prone
- [ ] Compile and run `legacy.cpp`
- [ ] Read `legacy.cpp` and understand how the hand-written `FibonacciIterator` works
- [ ] Understand the state machine: `a_`, `b_`, and `remaining_` track where we are
- [ ] Try to compile `modern.cpp` (will likely fail — modern stdlib needed)
- [ ] If `modern.cpp` compiled: run it and see how much simpler the `fibonacci()` function is
- [ ] Understand `co_yield` — it is like `return`, but the function remembers where it stopped
- [ ] Understand `std::views::take(10)` — how it lazily limits the sequence without building it all
- [ ] If you need to generate sequences: sketch how you would write a generator for your problem

---

## Final Projects (Pick One or More)

Choose a small project to apply what you learned:

### Project A: Config Parser
- [ ] Write a function that reads a config file line-by-line
- [ ] Use `std::expected` to report parse errors
- [ ] Use `and_then()` to chain multiple parse steps
- [ ] Use `std::print` for error messages

### Project B: Generic Container Wrapper
- [ ] Write a template class that wraps a `std::vector`
- [ ] Use deducing-`this` to write a single `get()` method that works for const and non-const
- [ ] Test it with both `const` and non-`const` instances

### Project C: Matrix Library
- [ ] Create a simple matrix class using `std::mdspan`
- [ ] Implement row and column iteration
- [ ] Write a helper function to fill the matrix with a pattern
- [ ] Compare performance vs. `vector<vector<T>>`

### Project D: Lazy Range
- [ ] Write a `std::generator` that produces a custom sequence (e.g., prime numbers, Fibonacci, or a pattern from your domain)
- [ ] Combine it with `std::views::filter()` and `std::views::take()`
- [ ] Measure memory use compared to eager generation

---

## Knowledge Check

Answer these without looking at the notes:

- [ ] What does `std::expected` carry that `std::optional` does not?
- [ ] Why do you need 4 overloads in pre-C++23 code, and how many do you need with deducing-`this`?
- [ ] Why is `std::print` safer than `printf`, and faster than `std::cout`?
- [ ] What is `std::mdspan`, and why is it better than `vector<vector<T>>`?
- [ ] What does `co_yield` do in a `std::generator`?

---

## Next Steps

Once you have completed this checklist:

1. **Read the proposals** — go to cppreference.com and read the official documentation for each feature
2. **Read others' code** — find open-source C++23 projects on GitHub and see how they use these features
3. **Experiment** — modify the examples, combine features, break them intentionally to understand the compiler errors
4. **Teach others** — explain one feature to a colleague; you will discover gaps in your understanding

Good luck!
