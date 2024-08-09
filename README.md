# Backstory

Originated as a scrappy struct to handle big integer operations to solve
kattis problems instead of reverting to using python. Ended up using it as a
project to better understand rvalue references and other computer science
things.

# Features

- Data member is not a string, it is a vector of 32 bit ints using all 2^32
  values per element.
- Can be constructed from and converted to an int, float, or string(base10 or
  hex representation).
- All operators are implemented.
- Karasuba and Toom3 multiplication optimizations.
- Pow function using exponentiation by squaring.
- Rvalue overloads on many operators to reduce unnecessary copies.
- `std::hash` specialization is implemented so you can use it as a key in a
  `std::unordered_map`.
- No macros, just plain C++20 and the standard library.
- Unit tests with near 100% coverage.

