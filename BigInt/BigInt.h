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
    explicit BigInt(double num);

    BigInt &operator+=(const BigInt &rhs);
    BigInt &operator+=(BigInt &&rhs);
    BigInt &operator-=(const BigInt &rhs);
    BigInt &operator-=(BigInt &&rhs);
    BigInt &operator*=(const BigInt &rhs);
    BigInt &operator/=(const BigInt &rhs);
    BigInt &operator/=(BigInt &&rhs);
    BigInt &operator%=(const BigInt &rhs);
    BigInt &operator%=(BigInt &&rhs);
    BigInt &operator&=(const BigInt &rhs);
    BigInt &operator&=(BigInt &&rhs);
    BigInt &operator|=(const BigInt &rhs);
    BigInt &operator|=(BigInt &&rhs);
    BigInt &operator^=(const BigInt &rhs);
    BigInt &operator^=(BigInt &&rhs);
    BigInt &operator<<=(std::int64_t n);
    BigInt &operator>>=(std::int64_t n);
    BigInt &operator++();
    BigInt &operator--();
    BigInt operator++(int);
    BigInt operator--(int);
    BigInt operator-() const &;
    BigInt &&operator-() &&;
    BigInt operator+(const BigInt &rhs) const &;
    BigInt &&operator+(BigInt &&rhs) const &;
    BigInt &&operator+(const BigInt &rhs) &&;
    BigInt &&operator+(BigInt &&rhs) &&;
    BigInt operator-(const BigInt &rhs) const &;
    BigInt &&operator-(BigInt &&rhs) const &;
    BigInt &&operator-(const BigInt &rhs) &&;
    BigInt &&operator-(BigInt &&rhs) &&;
    BigInt operator*(const BigInt &rhs) const;
    BigInt operator/(const BigInt &rhs) const &;
    BigInt operator/(BigInt &&rhs) const &;
    BigInt operator/(const BigInt &rhs) &&;
    BigInt operator/(BigInt &&rhs) &&;
    BigInt operator%(const BigInt &rhs) const &;
    BigInt operator%(BigInt &&rhs) const &;
    BigInt operator%(const BigInt &rhs) &&;
    BigInt operator%(BigInt &&rhs) &&;
    BigInt operator~() const &;
    BigInt &&operator~() &&;
    BigInt operator&(const BigInt &rhs) const &;
    BigInt &&operator&(BigInt &&rhs) const &;
    BigInt &&operator&(const BigInt &rhs) &&;
    BigInt &&operator&(BigInt &&rhs) &&;
    BigInt operator|(const BigInt &rhs) const &;
    BigInt &&operator|(BigInt &&rhs) const &;
    BigInt &&operator|(const BigInt &rhs) &&;
    BigInt &&operator|(BigInt &&rhs) &&;
    BigInt operator^(const BigInt &rhs) const &;
    BigInt &&operator^(BigInt &&rhs) const &;
    BigInt &&operator^(const BigInt &rhs) &&;
    BigInt &&operator^(BigInt &&rhs) &&;
    BigInt operator<<(std::int64_t n) const &;
    BigInt &&operator<<(std::int64_t n) &&;
    BigInt operator>>(std::int64_t n) const &;
    BigInt &&operator>>(std::int64_t n) &&;
    explicit operator bool() const;
    bool operator==(const BigInt &rhs) const;
    std::strong_ordering operator<=>(const BigInt &rhs) const &;
    std::strong_ordering operator<=>(BigInt &&rhs) const &;
    std::strong_ordering operator<=>(const BigInt &rhs) &&;
    std::strong_ordering operator<=>(BigInt &&rhs) &&;

    void negate();
    void invert();

    std::int64_t toInteger() const;
    double toDouble() const;
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
