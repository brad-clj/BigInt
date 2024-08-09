#pragma once
#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct DivModRes;

struct BigInt
{
    std::vector<std::uint32_t> chunks;
    bool isNeg = false;

    BigInt();
    BigInt(int num);
    BigInt(long num);
    BigInt(long long num);
    BigInt(unsigned num);
    BigInt(unsigned long num);
    BigInt(unsigned long long num);
    explicit BigInt(float num);
    explicit BigInt(double num);
    explicit BigInt(long double num);

    BigInt &operator+=(const BigInt &other);
    BigInt &operator+=(BigInt &&other);
    BigInt &operator-=(const BigInt &other);
    BigInt &operator-=(BigInt &&rhs);
    BigInt &operator*=(const BigInt &other);
    BigInt &operator/=(const BigInt &other);
    BigInt &operator/=(BigInt &&other);
    BigInt &operator%=(const BigInt &other);
    BigInt &operator%=(BigInt &&other);
    BigInt &operator&=(const BigInt &other);
    BigInt &operator&=(BigInt &&other);
    BigInt &operator|=(const BigInt &other);
    BigInt &operator|=(BigInt &&other);
    BigInt &operator^=(const BigInt &other);
    BigInt &operator^=(BigInt &&other);
    BigInt &operator<<=(std::int64_t n);
    BigInt &operator>>=(std::int64_t n);
    BigInt &operator++();
    BigInt &operator--();
    BigInt operator++(int);
    BigInt operator--(int);
    BigInt operator-() const &;
    BigInt operator-() &&;
    BigInt operator~() const &;
    BigInt operator~() &&;
    explicit operator bool() const;

    void normalize();
    void negate();
    void invert();

    std::int64_t toInteger() const;
    float toFloat() const;
    double toDouble() const;
    long double toLongDouble() const;
    std::string toString() const &;
    std::string toString() &&;
    std::string toHex() const;

    static BigInt fromString(std::string_view str);
    static BigInt fromHex(std::string_view str);
    static DivModRes divmod(const BigInt &lhs, const BigInt &rhs);
    static DivModRes divmod(const BigInt &lhs, BigInt &&rhs);
    static DivModRes divmod(BigInt &&lhs, const BigInt &rhs);
    static DivModRes divmod(BigInt &&lhs, BigInt &&rhs);
    static BigInt pow(const BigInt &base, std::int64_t exp);
};

struct DivModRes
{
    BigInt q;
    BigInt r;
};

BigInt operator+(const BigInt &lhs, const BigInt &rhs);
BigInt operator+(const BigInt &lhs, BigInt &&rhs);
BigInt operator+(BigInt &&lhs, const BigInt &rhs);
BigInt operator+(BigInt &&lhs, BigInt &&rhs);
BigInt operator-(const BigInt &lhs, const BigInt &rhs);
BigInt operator-(const BigInt &lhs, BigInt &&rhs);
BigInt operator-(BigInt &&lhs, const BigInt &rhs);
BigInt operator-(BigInt &&lhs, BigInt &&rhs);
BigInt operator*(const BigInt &lhs, const BigInt &rhs);
BigInt operator/(const BigInt &lhs, const BigInt &rhs);
BigInt operator/(const BigInt &lhs, BigInt &&rhs);
BigInt operator/(BigInt &&lhs, const BigInt &rhs);
BigInt operator/(BigInt &&lhs, BigInt &&rhs);
BigInt operator%(const BigInt &lhs, const BigInt &rhs);
BigInt operator%(const BigInt &lhs, BigInt &&rhs);
BigInt operator%(BigInt &&lhs, const BigInt &rhs);
BigInt operator%(BigInt &&lhs, BigInt &&rhs);
BigInt operator&(const BigInt &lhs, const BigInt &rhs);
BigInt operator&(const BigInt &lhs, BigInt &&rhs);
BigInt operator&(BigInt &&lhs, const BigInt &rhs);
BigInt operator&(BigInt &&lhs, BigInt &&rhs);
BigInt operator|(const BigInt &lhs, const BigInt &rhs);
BigInt operator|(const BigInt &lhs, BigInt &&rhs);
BigInt operator|(BigInt &&lhs, const BigInt &rhs);
BigInt operator|(BigInt &&lhs, BigInt &&rhs);
BigInt operator^(const BigInt &lhs, const BigInt &rhs);
BigInt operator^(const BigInt &lhs, BigInt &&rhs);
BigInt operator^(BigInt &&lhs, const BigInt &rhs);
BigInt operator^(BigInt &&lhs, BigInt &&rhs);
BigInt operator<<(const BigInt &lhs, std::int64_t rhs);
BigInt operator<<(BigInt &&lhs, std::int64_t rhs);
BigInt operator>>(const BigInt &lhs, std::int64_t rhs);
BigInt operator>>(BigInt &&lhs, std::int64_t rhs);
bool operator==(const BigInt &lhs, const BigInt &rhs);
std::strong_ordering operator<=>(const BigInt &lhs, const BigInt &rhs);
std::strong_ordering operator<=>(const BigInt &lhs, BigInt &&rhs);
std::strong_ordering operator<=>(BigInt &&lhs, const BigInt &rhs);
std::strong_ordering operator<=>(BigInt &&lhs, BigInt &&rhs);

template <>
struct std::hash<BigInt>
{
    std::size_t operator()(const BigInt &val) const noexcept;
};
