# Multidimensional `operator[]` and `std::mdspan`

**Proposals:** [P2128R6](https://wg21.link/P2128R6) (multiple subscript operands) and [P0009R18](https://wg21.link/P0009R18) (`std::mdspan`) — both in C++23.

## 1. The problem in older standards

Before C++23, `operator[]` could take **exactly one** argument (inherited from C, where arrays are one-dimensional). For multidimensional data (matrices, images, multi-channel sensor buffers — exactly the kind of data you see in signal processing or a multi-channel CANopen PDO), developers had to pick one of these:

1. **`operator()(i, j)` instead of `operator[]`**: works, but is semantically misleading — `()` usually means "call," not "access an element," and it looks inconsistent with standard containers.
2. **`operator[](i)[j]`**: requires the type to be an array-of-arrays (`T**` or `vector<vector<T>>`). This means each row is a separate heap allocation — cache-unfriendly (memory is not contiguous) and adds allocation overhead per row.
3. **Manual flat-index arithmetic**: `data[row * numCols + col]` — works and is cache-friendly, but has **no type safety**. Forgetting the order of `row`/`col`, or getting the stride formula wrong, produces a silent bug that the compiler will never catch.

Also, before C++23 there was no standard **view** for "look at an existing contiguous buffer as multidimensional data" — you could not treat a raw `float*` as a type-safe 2D matrix without copying it.

## 2. How the new features work

**(a) Multidimensional `operator[]`** — the signature of `operator[]` can now take multiple parameters:

```cpp
T& operator[](std::size_t row, std::size_t col);
```

This is a **core-language change only**; it extends the `[]` grammar from a single argument to a comma-separated argument list. Code that already used `a[i][j]` (two separate subscripts) does not need to change, but library authors can now also write `a[i, j]`.

**(b) `std::mdspan<T, Extents>`** — a **non-owning view** over an existing contiguous buffer (like `std::span`, but multidimensional):

- **No data copy**: it only holds a pointer plus size/stride metadata.
- **Configurable layout policy**: `layout_right` (row-major / C-style, the default), `layout_left` (column-major / Fortran-style), or a custom `layout_stride` — without changing the element-access code.
- **Extents can be static or dynamic**: if the dimensions are known at compile time (`std::extents<size_t, 3, 3>`), the compiler can fully inline the index computation — with zero overhead compared to a hand-written array.
- Combines with the new multidimensional `operator[]`: `view[i, j]` performs exactly the same computation as `data[i*cols+j]`, but type-safe and with bounds that can be checked in debug builds.

## 3. Comparing the examples

`examples/legacy.cpp`: a matrix built with `std::vector<std::vector<double>>` (heap allocation per row), and an alternative built with manual flat-index arithmetic on a flat buffer (no type safety).

`examples/modern.cpp`: the same matrix using `std::mdspan` over a flat `std::vector<double>`, accessed with `view[i, j]`, with no copy and no extra heap allocation.

## 4. Technical summary

| Criterion | `vector<vector<T>>` | Manual flat index | `std::mdspan` + `operator[i,j]` |
|---|---|---|---|
| Contiguous memory (cache-friendly) | no | yes | yes |
| Extra heap allocation per row | yes | no | no (it's a view, not an owner) |
| Type safety of multidimensional access | medium | no | high |
| Can change layout without changing access code | no | no | yes (`layout_left/right/stride`) |
| Runtime cost vs. manual access | higher (indirection) | zero (baseline) | zero (inlined at `-O2`) |
