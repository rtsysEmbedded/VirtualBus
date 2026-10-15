# `std::generator<T>` — Lazy Coroutine-Based Sequences

**Proposal:** [P2502R2](https://wg21.link/P2502R2) — accepted in C++23, header `<generator>` (built on coroutines, which arrived in C++20).

## 1. The problem in older standards

Generating a **lazy sequence** (values computed one-at-a-time, only when needed, not all up front) in C++17 and earlier had three incomplete options:

1. **Eager generation and store in `std::vector`**: the entire sequence is computed first, even if the caller only needs the first few values — wasting memory and CPU. For infinite sequences (like "all prime numbers"), this approach is impossible.
2. **Write a custom iterator class by hand**: you must manually implement `operator++`, `operator*`, `operator==`, and state-tracking. For complex logic (e.g., recursively traversing a tree), the state must be stored on the heap (because ordinary function calls cannot "pause" and resume later) — complex code, error-prone, hard to maintain.
3. **Callback/visitor pattern**: `void traverse(std::function<void(T) callback)` — works, but inverts control flow (the caller cannot easily say "stop iterating" or combine two traversals with `zip`).

## 2. How `std::generator` works

`std::generator<T>` is a standard coroutine return type. A function that returns `std::generator<T>` and uses `co_yield` is automatically transformed by the compiler into a state machine:

- **Lazy execution**: the function body runs only up to the first `co_yield`; the rest executes only when the caller requests the next value (via `++it`).
- **Automatic state storage**: the coroutine frame (a compiler-generated structure on the heap) holds all local variables and the program counter. The programmer does not manually manage state.
- **Compatible with ranges**: since `std::generator<T>` models an `input_range`, you can use it directly in range-based `for` loops and combine it with `std::views` (`filter`, `take`, etc.).
- **Natural for infinite sequences**: writing a generator for "all Fibonacci numbers" or "all primes" is straightforward; only the caller decides how many to actually consume (e.g., with `std::views::take(n)`).
- **Cost**: typically one heap allocation for the coroutine frame. This cost is acceptable for the dramatic simplification, but in hard real-time code (e.g., an interrupt handler) you should be aware.

## 3. Comparing the examples

`examples/legacy.cpp`: generating a Fibonacci sequence two ways — (a) eager in `std::vector`, (b) a hand-written iterator that stores state explicitly.

`examples/modern.cpp`: the same sequence as a `std::generator<unsigned long long>` with `co_yield`, consumed with a range-based `for` loop and combined with `std::views::take`.

## 4. Technical summary

| Criterion | Eager (`vector`) | Hand-written iterator | `std::generator` + `co_yield` |
|---|---|---|---|
| Supports infinite sequences | no | yes (with complex code) | yes (natural) |
| Code complexity for complex logic | low (but eager) | high | low |
| State management | manual (if dynamic) | manual and explicit | automatic (coroutine frame) |
| Composition with `std::views` | yes (after building all) | requires custom adapters | yes, direct |
| Memory cost for large sequences | high (whole sequence) | low | low (just one element + frame) |
