# Deducing `this` (Explicit Object Parameters)

**Proposal:** [P0847R7](https://wg21.link/P0847R7) — accepted in C++23.

## 1. The problem in older standards

Before C++23, the implicit `this` parameter that every member function receives **could not be deduced by templates**, and had to be covered by a separate overload for every combination of cv-qualifier (`const`/non-const) and ref-qualifier (`&`/`&&`). This caused three real problems:

1. **Boilerplate**: to write a getter that works correctly on lvalues and rvalues, both const and non-const, you had to write 4 overloads:
   `T& get() &`, `const T& get() const &`, `T&& get() &&`, `const T&& get() const &&`.
2. **CRTP (Curiously Recurring Template Pattern) for static polymorphism**: for a base class to call a method on the derived class without virtual dispatch, the base class had to be a template, and the derived object recovered via `static_cast<Derived*>(this)` — a pattern that is hard to read, produces long error messages, and requires advanced template-metaprogramming knowledge.
3. **Recursive lambdas**: up to C++20, a lambda could not call itself directly, because it has no name to refer to itself with. The common workaround was `std::function` (with the cost of type erasure and heap allocation), or a manual Y-combinator.

## 2. How deducing `this` works

C++23 lets you write the first parameter of a member function **explicitly**, marked with the `this` keyword:

```cpp
struct Widget {
    template <typename Self>
    auto&& get(this Self&& self) { return std::forward<Self>(self).value; }
};
```

Key technical points:

- The compiler deduces the type `Self` from the actual type of the calling object — exactly like a universal reference in a free function. This means one single function can replace all 4 overloads above.
- **CRTP without a templated base**: the base class no longer needs to be a template. The base method is written with `this Self&& self`, and `Self` is resolved at the call site to the actual derived type — no manual `static_cast` and no need to forward-declare the derived type.
- **Recursive lambdas**: a lambda can take its own first parameter as `this auto&& self` and call itself with `self(...)` — no `std::function`, no type-erasure cost.
- ABI-wise, this is a real, nameable parameter, not an implicit pointer. So the usual template argument deduction rules (including `const`/`&`/`&&`) apply to it.

## 3. Comparing the examples

`examples/legacy.cpp`:
- A getter implemented with 4 manual overloads to fully cover cv/ref-qualifiers.
- A classic CRTP implementation for static polymorphism (templated base class + `static_cast<Derived*>(this)`).
- A recursive lambda implemented with `std::function` (heap allocation cost).

`examples/modern.cpp`:
- The same getter with a single member template using `this Self&&`.
- The same CRTP pattern without templating the base class.
- The same recursive lambda using `this auto&& self`, no `std::function` needed.

## 4. Technical summary

| Criterion | Old approach | Deducing `this` |
|---|---|---|
| Overloads needed for full cv/ref coverage | 4 | 1 |
| Base class must be templated for CRTP | yes | no |
| Recursive lambda without heap allocation | no (`std::function`) | yes |
| Readability of generic code | low (four near-identical versions) | high (one definition) |
| Risk of overloads drifting out of sync when logic changes | high | zero (a single change point) |
