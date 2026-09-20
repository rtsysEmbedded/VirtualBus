# Deducing `this` (Explicit Object Parameters)

**Proposal:** [P0847R7](https://wg21.link/P0847R7) — پذیرفته‌شده در C++23.

## ۱. مسئله در نسخه‌های قبلی

پیش از C++23، پارامتر ضمنی `this` که هر متد عضو دریافت می‌کند **قابل template-deduce شدن نیست** و باید برای هر ترکیب از cv-qualifier (`const`/non-const) و ref-qualifier (`&`/`&&`) یک overload جداگانه نوشته شود. این باعث سه مشکل واقعی می‌شود:

1. **تکرار کد (boilerplate)**: برای پیاده‌سازی یک getter که هم روی lvalue و هم rvalue، هم const و هم non-const درست کار کند، باید ۴ overload بنویسید:
   `T& get() &`, `const T& get() const &`, `T&& get() &&`, `const T&& get() const &&`.
2. **CRTP (Curiously Recurring Template Pattern) برای static polymorphism**: برای اینکه یک کلاس پایه به متد مشتق‌شده دسترسی پیدا کند بدون virtual dispatch، باید کلاس پایه را template کرد و مشتق را از طریق `static_cast<Derived*>(this)` بازیابی کرد — الگویی پیچیده، با پیام‌های خطای طولانی، و نیازمند دانش پیشرفته از template metaprogramming.
3. **پیاده‌سازی recursive lambda**: تا C++20، lambda نمی‌تواند خودش را مستقیم صدا بزند چون نامی برای ارجاع به خودش ندارد؛ راه‌حل رایج استفاده از `std::function` (با هزینه‌ی type-erasure و heap allocation) یا `Y-combinator` دستی بود.

## ۲. مکانیزم Deducing `this`

C++23 اجازه می‌دهد اولین پارامتر یک متد عضو **به‌صراحت** نوشته شود و با کلیدواژه‌ی `this` علامت‌گذاری شود:

```cpp
struct Widget {
    template <typename Self>
    auto&& get(this Self&& self) { return std::forward<Self>(self).value; }
};
```

نکات فنی:

- کامپایلر نوع `Self` را از نوع واقعی شیءِ فراخوان‌کننده استنتاج می‌کند — دقیقاً مثل universal reference در توابع آزاد. این یعنی یک تابع واحد می‌تواند جایگزین تمام ۴ overload بالا شود.
- **CRTP بدون template پایه**: کلاس پایه دیگر نیازی به template‌شدن ندارد؛ متد پایه با `this Self&& self` نوشته می‌شود و `Self` در زمان فراخوان به نوع مشتق‌شده‌ی واقعی resolve می‌شود — بدون `static_cast` دستی و بدون نیاز به forward-declare کردن مشتق.
- **Recursive lambda**: lambda می‌تواند اولین پارامتر خودش را `this auto&& self` بگیرد و با `self(...)` خودش را صدا بزند — بدون `std::function` و بدون هزینه‌ی type erasure.
- از نظر ABI، این یک پارامتر واقعی (nameable) است، نه یک پوینتر ضمنی؛ به همین دلیل قوانین معمول template argument deduction (شامل `const`/`&`/`&&`) روی آن اعمال می‌شود.

## ۳. مقایسه در مثال‌ها

`examples/legacy.cpp`:
- پیاده‌سازی getter با ۴ overload دستی برای پوشش کامل cv/ref-qualifier.
- پیاده‌سازی CRTP سنتی برای static polymorphism (کلاس پایه‌ی template‌شده + `static_cast<Derived*>(this)`).
- پیاده‌سازی recursive lambda با `std::function` (هزینه‌ی heap allocation).

`examples/modern.cpp`:
- همان getter با یک تابع template واحد با `this Self&&`.
- همان CRTP بدون template کردن کلاس پایه.
- همان recursive lambda با `this auto&& self`، بدون `std::function`.

## ۴. جمع‌بندی فنی

| معیار | روش قدیمی | Deducing `this` |
|---|---|---|
| تعداد overload برای پوشش کامل cv/ref | ۴ | ۱ |
| نیاز به template کردن کلاس پایه برای CRTP | بله | خیر |
| Recursive lambda بدون heap allocation | خیر (`std::function`) | بله |
| خوانایی کد generic | پایین (چهار نسخه‌ی تقریباً یکسان) | بالا (یک تعریف) |
| خطر عدم‌همگام‌سازی overload‌ها هنگام تغییر منطق | بالا | صفر (یک نقطه‌ی تغییر) |
