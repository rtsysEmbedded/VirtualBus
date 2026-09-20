# C++23 — راهنمای یادگیری فنی و عمیق

این بخش برای یادگیری قابلیت‌های جدید استاندارد **C++23** (آخرین نسخه‌ی کاملاً منتشرشده و پشتیبانی‌شده‌ی گسترده تا تاریخ نگارش این سند) ساخته شده است. هدف این نیست که فقط سینتکس جدید نشان داده شود؛ برای هر قابلیت:

1. **مسئله‌ی زمینه‌ای (Motivation)** — چرا کمیته‌ی استاندارد این قابلیت را اضافه کرد و چه مشکلی در نسخه‌های قبلی وجود داشت.
2. **مکانیزم دقیق زبان/کتابخانه** — رفتار در سطح کامپایلر، تغییرات در overload resolution، ABI، یا کتابخانه‌ی استاندارد.
3. **کد قدیمی (Legacy, تا C++17)** — پیاده‌سازی معادل بدون قابلیت جدید، به همراه معایب مشخص (boilerplate، هزینه‌ی runtime، UB احتمالی، ضعف در type-safety).
4. **کد جدید (C++23)** — همان مسئله با استفاده از قابلیت جدید.
5. **تحلیل مقایسه‌ای** — کارایی، خوانایی، ایمنی نوع (type safety)، و اثر روی ABI/باینری.

هر مثال یک فایل `.cpp` **مستقل و قابل کامپایل** است (نه اسنیپت داخل Markdown) تا بتوان صحت ادعاها را با اجرای واقعی تأیید کرد.

## پیش‌نیاز کامپایلر

| قابلیت | حداقل GCC | حداقل Clang |
|---|---|---|
| `std::expected` | GCC 12 (کامل در 13) | Clang 16 |
| Deducing `this` | GCC 14 | Clang 18 |
| `std::print` / `std::println` | GCC 14 (نیاز به libstdc++ با `-lfmt` در برخی توزیع‌ها) | Clang 17+ با libc++ |
| Multidimensional `operator[]` | GCC 12 | Clang 12 |
| `std::mdspan` | GCC 15 | Clang 18 |
| `std::generator` (coroutines) | GCC 14 با `-fcoroutines` | Clang 17 |

> **نکته‌ی فنی مهم:** پشتیبانی کتابخانه‌ی استاندارد (libstdc++/libc++) از پشتیبانی خودِ کامپایلر از زبان عقب‌تر است. اگر کامپایلر شما `-std=c++23` را می‌پذیرد، به این معنا نیست که `<expected>` یا `<print>` را دارد. قبل از اجرا، نسخه‌ی کامپایلر خود را با جدول بالا مقایسه کنید.

### وضعیت واقعی تست‌شده در این محیط توسعه

تمام مثال‌های `legacy.cpp` و همچنین `01-std-expected/modern.cpp` و `02-deducing-this/modern.cpp` روی `g++ 13.3.0` و/یا `clang++ 18.1.3` (هر دو با کتابخانه‌ی `libstdc++`، بدون `libc++`) **کامپایل و اجرا شدند و خروجی مورد انتظار را تولید کردند**. `build.py` به‌صورت خودکار هر دو کامپایلر را امتحان می‌کند (چون deducing-this زبانی است و روی `g++ 13` که فقط تا حدی از قابلیت‌های core-language C++23 پشتیبانی می‌کند شکست می‌خورد، اما روی `clang++ 18` موفق است).

مثال‌های `03-std-print/modern.cpp`، `04-multidim-subscript-mdspan/modern.cpp` و `05-std-generator/modern.cpp` در این محیط **کامپایل نشدند** چون `libstdc++ 13` هدرهای `<print>`, `<mdspan>`, `<generator>` را ندارد (این هدرها به libstdc++ 14/15 نیاز دارند) و `libc++` روی این ماشین نصب نیست. کد این سه مثال بر اساس مستندات رسمی cppreference.com و متن پذیرفته‌شده‌ی proposal مربوطه نوشته شده و از نظر سینتکسی با کامپایلرهایی که این هدرها را دارند (مثلاً GCC 14+ یا Clang 18+ با `libc++`) باید بدون تغییر کامپایل شوند؛ اما تا نصب چنین toolchain‌ای، این ادعا برای شما «تأییدشده با اجرا» نیست — این محدودیت را صادقانه اعلام می‌کنیم تا اطلاعات نادرست به‌عنوان تست‌شده معرفی نشود.

## اجرای مثال‌ها

تمام تنظیمات build (کامپایلر، فلگ‌های `-std=`، نسخه‌ی قدیمی/جدید هر مثال) در فایل JSON زیر تعریف شده‌اند — **هیچ مقداری در اسکریپت یا کد هاردکد نشده است**:

```
docs/learning/cpp23-features/config/features.json
```

اجرای همه‌ی مثال‌ها:

```bash
python3 docs/learning/cpp23-features/build.py
```

اجرای یک قابلیت مشخص:

```bash
python3 docs/learning/cpp23-features/build.py 01-std-expected
```

## فهرست قابلیت‌ها

| # | قابلیت | Proposal | پوشه |
|---|---|---|---|
| 1 | `std::expected<T, E>` — مدیریت خطا بدون exception | [P0323R12](01-std-expected/README.md) | [01-std-expected](01-std-expected/README.md) |
| 2 | Deducing `this` (explicit object parameters) | [P0847R7](02-deducing-this/README.md) | [02-deducing-this](02-deducing-this/README.md) |
| 3 | `std::print` / `std::println` | [P2093R14](03-std-print/README.md) | [03-std-print](03-std-print/README.md) |
| 4 | Multidimensional `operator[]` و `std::mdspan` | [P2128R6](04-multidim-subscript-mdspan/README.md) | [04-multidim-subscript-mdspan](04-multidim-subscript-mdspan/README.md) |
| 5 | `std::generator<T>` (کوروتین‌های lazy) | [P2502R2](05-std-generator/README.md) | [05-std-generator](05-std-generator/README.md) |

هر پوشه شامل `README.md` (توضیح فنی کامل) و `examples/legacy.cpp` + `examples/modern.cpp` است.
