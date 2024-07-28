#pragma once
#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct DivModRes;

struct BigInt
{
    std::vector<uint32_t> data;
    bool negative = false;

    BigInt();
    BigInt(int64_t num);
    BigInt(std::string_view str);

    BigInt &operator+=(const BigInt &rhs);
    BigInt &operator+=(BigInt &&rhs);
    BigInt &operator-=(const BigInt &rhs);
    BigInt &operator-=(BigInt &&rhs);
    BigInt &operator*=(const BigInt &rhs);
    BigInt &operator/=(const BigInt &rhs);
    BigInt &operator%=(const BigInt &rhs);
    BigInt &operator&=(const BigInt &rhs);
    BigInt &operator&=(BigInt &&rhs);
    BigInt &operator|=(const BigInt &rhs);
    BigInt &operator|=(BigInt &&rhs);
    BigInt &operator^=(const BigInt &rhs);
    BigInt &operator^=(BigInt &&rhs);
    BigInt &operator<<=(const size_t n);
    BigInt &operator>>=(const size_t n);
    BigInt &operator++();
    BigInt &operator--();
    BigInt operator++(int);
    BigInt operator--(int);
    BigInt operator-() const &;
    BigInt operator-() &&;
    BigInt operator+(const BigInt &rhs) const &;
    BigInt operator+(BigInt &&rhs) const &;
    BigInt operator+(const BigInt &rhs) &&;
    BigInt operator+(BigInt &&rhs) &&;
    BigInt operator-(const BigInt &rhs) const &;
    BigInt operator-(BigInt &&rhs) const &;
    BigInt operator-(const BigInt &rhs) &&;
    BigInt operator-(BigInt &&rhs) &&;
    BigInt operator*(const BigInt &rhs) const;
    BigInt operator/(const BigInt &rhs) const;
    BigInt operator%(const BigInt &rhs) const;
    BigInt operator~() const &;
    BigInt operator~() &&;
    BigInt operator&(const BigInt &rhs) const &;
    BigInt operator&(BigInt &&rhs) const &;
    BigInt operator&(const BigInt &rhs) &&;
    BigInt operator&(BigInt &&rhs) &&;
    BigInt operator|(const BigInt &rhs) const &;
    BigInt operator|(BigInt &&rhs) const &;
    BigInt operator|(const BigInt &rhs) &&;
    BigInt operator|(BigInt &&rhs) &&;
    BigInt operator^(const BigInt &rhs) const &;
    BigInt operator^(BigInt &&rhs) const &;
    BigInt operator^(const BigInt &rhs) &&;
    BigInt operator^(BigInt &&rhs) &&;
    BigInt operator<<(const size_t n) const &;
    BigInt operator<<(const size_t n) &&;
    BigInt operator>>(const size_t n) const &;
    BigInt operator>>(const size_t n) &&;
    explicit operator bool() const;
    bool operator==(const BigInt &rhs) const;
    std::strong_ordering operator<=>(const BigInt &rhs) const;

    std::string toString() const;
    std::string toHex() const;
    void normalize();
    void negate();
    void invert();
    void addChunk(size_t i, const uint32_t val);
    void subChunk(size_t i, const uint32_t val);

    static BigInt fromHex(std::string_view str);
    static DivModRes divmod(const BigInt &lhs, const BigInt &rhs);
    static BigInt toom2(const BigInt &lhs, const BigInt &rhs);
    static BigInt toom3(const BigInt &lhs, const BigInt &rhs);
};

struct DivModRes
{
    BigInt q;
    BigInt r;
};
