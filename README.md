# BigInt

## Features

- Data member is not a string, it is a vector of 32 bit ints using all 2^32
  values per element.
- Can be constructed from and converted to an int, float, or string (base10 or
  hex representation).
- All operators are implemented.
- Karasuba and Toom3 multiplication optimizations.
- Pow function using exponentiation by squaring.
- Rvalue overloads on many operators to reduce unnecessary copies.
- `std::hash` specialization is implemented so you can use it as a key in a
  `std::unordered_map`.
- No macros, just plain C++20 and the standard library.
- Unit tests with near 100% coverage.

## Usage

Should be straight forward, just a
[header](https://github.com/brad-clj/BigInt/blob/main/BigInt/BigInt.h) and
[source](https://github.com/brad-clj/BigInt/blob/main/BigInt/BigInt.cpp) file.
If you want to use it in a single file like a Kattis submission just concatenate
the two and remove the `#pragma once` and move all the includes to the top.

You can find many usage examples in the
[tests](https://github.com/brad-clj/BigInt/blob/main/BigIntTest/BigIntTest.cpp).
Also to note the tests make it appear that you need to always call the
constructor even when using a standard integer type. But the constructor will
work as a converting constructor, just not the floating-point types as they are
marked as `explicit`. The idea behind that is there is a chance of loss of data.

```cpp
// example:
BigInt x(42);
auto y = 99 + x;          // y is now BigInt(141)
auto z = y - BigInt(41.0) // the constructor is required. z is BigInt(100)
```

I decided against using user-defined conversion functions for extracting a
fundamental type from a `BigInt`. So to do so I instead provided the methods
`toInteger`, `toFloat`, `toDouble`, and `toLongDouble`. `toInteger` returns a
`std::int64_t` of the least significant 64 bits as two's complement, so if the
value exceeds that expect it to be truncated. The floating point conversions
will contain the most significant portion of the value that fits in the float
unless it exceeds the range that the float can contain and then it will be
infinite.

For string operations there are the static methods `BigInt::fromString` and
`BigInt::fromHex`, and the methods `toString` and `toHex`. There are no
`ostream` and `istream` overloads they are outside the scope of my desires with
this project. The string representation has no prefix for positive and a hyphen
`-` prefix for negative, for hex you always need the prefix `0x` or `-0x` for
positive and negative respectively. If you are familiar with Python the hex
representation for a negative is like how you would see it there, which is what
the value would be if multiplied by `-1` and prefixed with a hyphen `-`. So if
you want to see the two's complement of it just bitwise `&` it to a large enough
`0xffff`. Also hex input and output is much faster than string (base 10), so if
you are feeding or emitting from your program large amounts of data try to have
it in hex if possible.

```cpp
// example:
std::string s;
std::cin >> s;
auto x = BigInt::fromString(s);
x += 99;
std::cout << x.toString() << '\n';
```

All the data members of `BigInt` are public, there is no private data or
methods, though there are a fair amount of static helpers in the source. If you
have a need to modify any data members outside of the methods implemented make
sure you call the method `normalize` after otherwise equality will fail to work
correctly.

## Backstory

Originated as a scrappy struct to handle integer operations that would exceed 64
bits when solving Kattis problems. It was probably easier to concede and just
use Python, but I was stubborn. I occasionally would do some web searching to
see if there was something that was reasonably performant but small and simple
enough that I could paste into a Kattis submission. I wasn't able to find
anything of such in my search, so I decided to clean up my scrappy struct. And
ended up using it as a project to better understand rvalue references and other
C++ things.
