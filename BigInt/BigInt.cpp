#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include "BigInt.h"

static const BigInt &Zero()
{
    static const BigInt val(0);
    return val;
}

static const BigInt &One()
{
    static const BigInt val(1);
    return val;
}

static const BigInt &TenQuintillion()
{
    static const BigInt val(10'000'000'000'000'000'000u);
    return val;
}

template <typename N, typename D>
auto ceilDiv(const N n, const D d) { return n / d + (n % d ? 1 : 0); }

static bool addChunk(std::vector<std::uint32_t> &chunks, std::size_t i, const std::uint32_t val)
{
    auto hasCarry = (chunks[i++] += val) < val;
    while (hasCarry && i < chunks.size())
    {
        hasCarry = ++chunks[i++] == 0;
    }
    return hasCarry;
}

static bool subChunk(std::vector<std::uint32_t> &chunks, std::size_t i, const std::uint32_t val)
{
    auto prev = chunks[i];
    auto hasBorrow = (chunks[i++] -= val) > prev;
    while (hasBorrow && i < chunks.size())
    {
        hasBorrow = --chunks[i++] == static_cast<std::uint32_t>(-1);
    }
    return hasBorrow;
}

static void normalize(BigInt &big)
{
    while (big.chunks.size() && big.chunks.back() == 0)
    {
        big.chunks.pop_back();
    }
    if (big.chunks.size() == 0)
        big.isNeg = false;
}

static void add(BigInt &acc, const BigInt &other)
{
    acc.chunks.resize(std::max(acc.chunks.size(), other.chunks.size()) + 1);
    for (std::size_t i = 0; i < other.chunks.size(); ++i)
    {
        if (other.chunks[i])
            addChunk(acc.chunks, i, other.chunks[i]);
    }
    normalize(acc);
}

static void sub(BigInt &acc, const BigInt &other)
{
    if (acc.chunks.size() < other.chunks.size())
        acc.chunks.resize(other.chunks.size());
    bool hasBorrow = false;
    for (std::size_t i = 0; i < other.chunks.size(); ++i)
    {
        if (other.chunks[i] && subChunk(acc.chunks, i, other.chunks[i]))
            hasBorrow = true;
    }
    if (hasBorrow)
    {
        for (auto &chunk : acc.chunks)
        {
            chunk = ~chunk;
        }
        addChunk(acc.chunks, 0, 1);
        acc.negate();
    }
    normalize(acc);
}

BigInt::BigInt() {}

BigInt::BigInt(int num) : BigInt(static_cast<long long>(num)) {}
BigInt::BigInt(long num) : BigInt(static_cast<long long>(num)) {}
BigInt::BigInt(const long long num) : BigInt(static_cast<unsigned long long>(num))
{
    if (num < 0)
    {
        bool borrow = true;
        for (auto &chunk : chunks)
        {
            if (borrow)
                borrow = --chunk == static_cast<std::uint32_t>(-1);
            chunk = ~chunk;
        }
        isNeg = true;
        normalize(*this);
    }
}

BigInt::BigInt(unsigned num) : BigInt(static_cast<unsigned long long>(num)) {}
BigInt::BigInt(unsigned long num) : BigInt(static_cast<unsigned long long>(num)) {}
BigInt::BigInt(unsigned long long num)
{
    chunks.reserve(2);
    while (num)
    {
        chunks.push_back(static_cast<std::uint32_t>(num));
        num >>= 32;
    }
}

template <typename T>
void floatConvert(BigInt &big, T t)
{
    if (!std::isnormal(t))
        return;
    int exp;
    auto fr = std::frexp(std::trunc(t), &exp);
    if (fr < 0.0)
    {
        fr = -fr;
        big.isNeg = true;
    }
    big.chunks.resize(ceilDiv(exp, 32));
    for (std::size_t i = big.chunks.size(); i--;)
    {
        auto &chunk = big.chunks[i];
        auto j = exp % 32;
        if (j == 0)
            j = 32;
        while (j--)
        {
            chunk <<= 1;
            if (fr >= 0.5)
            {
                chunk |= 0x1;
                fr -= 0.5;
                if (fr == 0.0)
                {
                    chunk <<= j;
                    return;
                }
            }
            fr *= 2.0;
            --exp;
        }
    }
}

BigInt::BigInt(float num) { floatConvert(*this, num); }
BigInt::BigInt(double num) { floatConvert(*this, num); }
BigInt::BigInt(long double num) { floatConvert(*this, num); }

BigInt &BigInt::operator+=(const BigInt &rhs)
{
    if (this == &rhs)
    {
        auto rhsCopy = rhs;
        return *this += rhsCopy;
    }
    if (isNeg == rhs.isNeg)
        add(*this, rhs);
    else
        sub(*this, rhs);
    return *this;
}

BigInt &BigInt::operator+=(BigInt &&rhs)
{
    if (rhs.chunks.capacity() > chunks.capacity())
        std::swap(*this, rhs);
    return *this += rhs;
}

BigInt &BigInt::operator-=(const BigInt &rhs)
{
    if (this == &rhs)
        return *this = Zero();
    if (isNeg != rhs.isNeg)
        add(*this, rhs);
    else
        sub(*this, rhs);
    return *this;
}

BigInt &BigInt::operator-=(BigInt &&rhs)
{
    if (rhs.chunks.capacity() > chunks.capacity())
    {
        std::swap(*this, rhs);
        negate();
        return *this += rhs;
    }
    return *this -= rhs;
}

BigInt &BigInt::operator*=(const BigInt &rhs) { return *this = *this * rhs; }

BigInt &BigInt::operator/=(const BigInt &rhs) { return *this = std::move(*this) / rhs; }
BigInt &BigInt::operator/=(BigInt &&rhs) { return *this = std::move(*this) / std::move(rhs); }
BigInt &BigInt::operator%=(const BigInt &rhs) { return *this = std::move(*this) % rhs; }
BigInt &BigInt::operator%=(BigInt &&rhs) { return *this = std::move(*this) % std::move(rhs); }

static void bitwise(BigInt &lhs, const BigInt &rhs, const std::function<void(std::uint32_t &, std::uint32_t)> &fn)
{
    std::uint32_t x = lhs.isNeg ? static_cast<std::uint32_t>(-1) : 0;
    fn(x, rhs.isNeg ? static_cast<std::uint32_t>(-1) : 0);
    bool resIsNeg = static_cast<bool>(x);
    lhs.chunks.resize(std::max(lhs.chunks.size(), rhs.chunks.size()) + (resIsNeg ? 1 : 0));
    bool lhsBorrow = lhs.isNeg;
    bool rhsBorrow = rhs.isNeg;
    bool resBorrow = resIsNeg;
    for (std::size_t i = 0; i < lhs.chunks.size(); ++i)
    {
        auto &a = lhs.chunks[i];
        if (lhs.isNeg)
        {
            if (lhsBorrow)
                lhsBorrow = --a == static_cast<std::uint32_t>(-1);
            a = ~a;
        }
        auto b = i < rhs.chunks.size() ? rhs.chunks[i] : 0;
        if (rhs.isNeg)
        {
            if (rhsBorrow)
                rhsBorrow = --b == static_cast<std::uint32_t>(-1);
            b = ~b;
        }
        fn(a, b);
        if (resIsNeg)
        {
            if (resBorrow)
                resBorrow = --a == static_cast<std::uint32_t>(-1);
            a = ~a;
        }
    }
    lhs.isNeg = resIsNeg;
    normalize(lhs);
}

BigInt &BigInt::operator&=(const BigInt &rhs)
{
    if (this == &rhs)
        return *this;
    bitwise(*this, rhs,
            [](std::uint32_t &a, std::uint32_t b)
            { a &= b; });
    return *this;
}

BigInt &BigInt::operator&=(BigInt &&rhs)
{
    if (rhs.chunks.capacity() > chunks.capacity())
        std::swap(*this, rhs);
    return *this &= rhs;
}

BigInt &BigInt::operator|=(const BigInt &rhs)
{
    if (this == &rhs)
        return *this;
    bitwise(*this, rhs,
            [](std::uint32_t &a, std::uint32_t b)
            { a |= b; });
    return *this;
}

BigInt &BigInt::operator|=(BigInt &&rhs)
{
    if (rhs.chunks.capacity() > chunks.capacity())
        std::swap(*this, rhs);
    return *this |= rhs;
}

BigInt &BigInt::operator^=(const BigInt &rhs)
{
    if (this == &rhs)
        return *this = Zero();
    bitwise(*this, rhs,
            [](std::uint32_t &a, std::uint32_t b)
            { a ^= b; });
    return *this;
}

BigInt &BigInt::operator^=(BigInt &&rhs)
{
    if (rhs.chunks.capacity() > chunks.capacity())
        std::swap(*this, rhs);
    return *this ^= rhs;
}

BigInt &BigInt::operator<<=(const std::int64_t n)
{
    if (n < 0)
        throw std::invalid_argument("BigInt operator<<= has negative shift");
    if (n == 0)
        return *this;
    const std::size_t off = n / 32;
    const auto s = n % 32;
    chunks.resize(chunks.size() + ceilDiv(n, 32));
    for (auto i = chunks.size(); i--;)
    {
        std::uint32_t x = 0;
        if (i >= off)
            x = chunks[i - off] << s;
        if (s && i >= off + 1)
            x |= chunks[i - (off + 1)] >> (32 - s);
        chunks[i] = x;
    }
    normalize(*this);
    return *this;
}

BigInt &BigInt::operator>>=(const std::int64_t n)
{
    if (n < 0)
        throw std::invalid_argument("BigInt operator>>= has negative shift");
    if (n == 0)
        return *this;
    if (static_cast<std::size_t>(n) >= chunks.size() * 32)
    {
        chunks.clear();
        if (isNeg)
            chunks.push_back(1);
        return *this;
    }
    if (isNeg)
        subChunk(chunks, 0, 1);
    const std::size_t off = n / 32;
    const auto s = n % 32;
    for (std::size_t i = 0; i < chunks.size(); ++i)
    {
        std::uint32_t x = 0;
        if (i + off < chunks.size())
            x = chunks[i + off] >> s;
        if (s && i + off + 1 < chunks.size())
            x |= chunks[i + off + 1] << (32 - s);
        chunks[i] = x;
    }
    if (isNeg)
        addChunk(chunks, 0, 1);
    normalize(*this);
    return *this;
}

BigInt &BigInt::operator++() { return *this += One(); }
BigInt &BigInt::operator--() { return *this -= One(); }

BigInt BigInt::operator++(int)
{
    auto res = *this;
    ++*this;
    return res;
}

BigInt BigInt::operator--(int)
{
    auto res = *this;
    --*this;
    return res;
}

BigInt BigInt::operator-() const &
{
    auto res = *this;
    res.negate();
    return res;
}

BigInt BigInt::operator-() &&
{
    negate();
    return std::move(*this);
}

BigInt BigInt::operator+(const BigInt &rhs) const &
{
    if (rhs.chunks.size() > chunks.size())
    {
        auto res = rhs;
        res += *this;
        return res;
    }
    auto res = *this;
    res += rhs;
    return res;
}

BigInt BigInt::operator+(BigInt &&rhs) const & { return std::move(rhs += *this); }
BigInt BigInt::operator+(const BigInt &rhs) && { return std::move(*this += rhs); }

BigInt BigInt::operator+(BigInt &&rhs) &&
{
    if (rhs.chunks.capacity() > chunks.capacity())
        return std::move(rhs += *this);
    return std::move(*this += rhs);
}

BigInt BigInt::operator-(const BigInt &rhs) const &
{
    if (rhs.chunks.size() > chunks.size())
    {
        auto res = rhs;
        res.negate();
        res += *this;
        return res;
    }
    if (this == &rhs)
        return Zero();
    auto res = *this;
    res -= rhs;
    return res;
}

BigInt BigInt::operator-(BigInt &&rhs) const &
{
    if (this == &rhs)
        return std::move(rhs = Zero());
    rhs.negate();
    return std::move(rhs += *this);
}

BigInt BigInt::operator-(const BigInt &rhs) && { return std::move(*this -= rhs); }

BigInt BigInt::operator-(BigInt &&rhs) &&
{
    if (rhs.chunks.capacity() > chunks.capacity())
        return *this - std::move(rhs);
    return std::move(*this -= rhs);
}

static BigInt mul(const BigInt &lhs, const BigInt &rhs)
{
    BigInt res;
    res.chunks.resize(lhs.chunks.size() + rhs.chunks.size() + 1);
    for (std::size_t i = 0; i < lhs.chunks.size(); ++i)
    {
        for (std::size_t j = 0; j < rhs.chunks.size(); ++j)
        {
            auto prod = static_cast<std::uint64_t>(lhs.chunks[i]) * rhs.chunks[j];
            if (prod)
                addChunk(res.chunks, i + j, static_cast<std::uint32_t>(prod));
            if (prod >> 32)
                addChunk(res.chunks, i + j + 1, static_cast<std::uint32_t>(prod >> 32));
        }
    }
    return res;
}

struct Toom2Split
{
    BigInt low, high;

    Toom2Split(const BigInt &big, const std::size_t sz)
    {
        auto iter1 = big.chunks.begin();
        auto iter2 = iter1 + std::min<std::ptrdiff_t>(sz, big.chunks.end() - iter1);
        auto iter3 = big.chunks.end();
        low.chunks = std::vector<std::uint32_t>(iter1, iter2);
        high.chunks = std::vector<std::uint32_t>(iter2, iter3);
        normalize(low);
        normalize(high);
    }
};

static BigInt toom2(const BigInt &lhs, const BigInt &rhs)
{
    const auto sz = ceilDiv(std::max(lhs.chunks.size(), rhs.chunks.size()), 2);
    Toom2Split p(lhs, sz);
    Toom2Split q(rhs, sz);
    std::array<BigInt, 3> r;
    r[0] = p.low * q.low;
    r[2] = p.high * q.high;
    r[1] = r[0] + r[2];
    r[1] -= (std::move(p.high) - std::move(p.low)) * (std::move(q.high) - std::move(q.low));
    BigInt res;
    res.chunks.resize(lhs.chunks.size() + rhs.chunks.size() + 1);
    for (std::size_t i = 0; i < r.size(); ++i)
    {
        for (std::size_t j = 0; j < r[i].chunks.size(); ++j)
        {
            if (r[i].chunks[j])
                addChunk(res.chunks, sz * i + j, r[i].chunks[j]);
        }
    }
    return res;
}

struct Toom3Mat
{
    BigInt zero, one, negone, negtwo, inf;

    Toom3Mat(const BigInt &big, const std::size_t sz)
    {
        BigInt b0, b1, b2;
        auto iter1 = big.chunks.begin();
        auto iter2 = iter1 + std::min<std::ptrdiff_t>(sz, big.chunks.end() - iter1);
        auto iter3 = iter2 + std::min<std::ptrdiff_t>(sz, big.chunks.end() - iter2);
        auto iter4 = big.chunks.end();
        b0.chunks = std::vector<std::uint32_t>(iter1, iter2);
        b1.chunks = std::vector<std::uint32_t>(iter2, iter3);
        b2.chunks = std::vector<std::uint32_t>(iter3, iter4);
        normalize(b0);
        normalize(b1);
        normalize(b2);
        auto tmp = b0 + b2;
        zero = b0;
        one = tmp + b1;
        negone = std::move(tmp) - std::move(b1);
        negtwo = ((negone + b2) << 1) - std::move(b0);
        inf = std::move(b2);
    }
};

static BigInt div2(BigInt &&big)
{
    static const BigInt negOne(-1);
    if (big == negOne)
        return Zero();
    else
        return std::move(big >>= 1);
}

static BigInt toom3(const BigInt &lhs, const BigInt &rhs)
{
    static const BigInt three(3);
    const auto sz = ceilDiv(std::max(lhs.chunks.size(), rhs.chunks.size()), 3);
    Toom3Mat p(lhs, sz);
    Toom3Mat q(rhs, sz);
    p.zero *= q.zero;
    p.one *= q.one;
    p.negone *= q.negone;
    p.negtwo *= q.negtwo;
    p.inf *= q.inf;
    std::array<BigInt, 5> r;
    r[0] = p.zero;
    r[4] = p.inf;
    r[3] = (std::move(p.negtwo) - p.one) / three;
    r[1] = div2(std::move(p.one) - p.negone);
    r[2] = std::move(p.negone) - std::move(p.zero);
    r[3] = div2(r[2] - r[3]) + (std::move(p.inf) << 1);
    r[2] += r[1] - r[4];
    r[1] -= r[3];
    BigInt res;
    res.chunks.resize(lhs.chunks.size() + rhs.chunks.size() + 1);
    for (std::size_t i = 0; i < r.size(); ++i)
    {
        for (std::size_t j = 0; j < r[i].chunks.size(); ++j)
        {
            if (r[i].chunks[j])
                addChunk(res.chunks, sz * i + j, r[i].chunks[j]);
        }
    }
    return res;
}

static constexpr std::size_t Toom2Thresh = 550;
static constexpr std::size_t Toom3Thresh = 2200;

BigInt BigInt::operator*(const BigInt &rhs) const
{
    const auto score = chunks.size() * rhs.chunks.size();
    auto res = score > Toom3Thresh   ? toom3(*this, rhs)
               : score > Toom2Thresh ? toom2(*this, rhs)
                                     : mul(*this, rhs);
    res.isNeg = isNeg != rhs.isNeg;
    normalize(res);
    return res;
}

BigInt BigInt::operator/(const BigInt &rhs) const & { return divmod(*this, rhs).q; }
BigInt BigInt::operator/(BigInt &&rhs) const & { return divmod(*this, std::move(rhs)).q; }
BigInt BigInt::operator/(const BigInt &rhs) && { return divmod(std::move(*this), rhs).q; }
BigInt BigInt::operator/(BigInt &&rhs) && { return divmod(std::move(*this), std::move(rhs)).q; }
BigInt BigInt::operator%(const BigInt &rhs) const & { return divmod(*this, rhs).r; }
BigInt BigInt::operator%(BigInt &&rhs) const & { return divmod(*this, std::move(rhs)).r; }
BigInt BigInt::operator%(const BigInt &rhs) && { return divmod(std::move(*this), rhs).r; }
BigInt BigInt::operator%(BigInt &&rhs) && { return divmod(std::move(*this), std::move(rhs)).r; }

BigInt BigInt::operator~() const &
{
    auto res = *this;
    res.invert();
    return res;
}

BigInt BigInt::operator~() &&
{
    invert();
    return std::move(*this);
}

BigInt BigInt::operator&(const BigInt &rhs) const &
{
    if (rhs.chunks.size() > chunks.size())
    {
        auto res = rhs;
        res &= *this;
        return res;
    }
    auto res = *this;
    res &= rhs;
    return res;
}

BigInt BigInt::operator&(BigInt &&rhs) const & { return std::move(rhs &= *this); }
BigInt BigInt::operator&(const BigInt &rhs) && { return std::move(*this &= rhs); }

BigInt BigInt::operator&(BigInt &&rhs) &&
{
    if (rhs.chunks.capacity() > chunks.capacity())
        return std::move(rhs &= *this);
    return std::move(*this &= rhs);
}

BigInt BigInt::operator|(const BigInt &rhs) const &
{
    if (rhs.chunks.size() > chunks.size())
    {
        auto res = rhs;
        res |= *this;
        return res;
    }
    auto res = *this;
    res |= rhs;
    return res;
}

BigInt BigInt::operator|(BigInt &&rhs) const & { return std::move(rhs |= *this); }
BigInt BigInt::operator|(const BigInt &rhs) && { return std::move(*this |= rhs); }

BigInt BigInt::operator|(BigInt &&rhs) &&
{
    if (rhs.chunks.capacity() > chunks.capacity())
        return std::move(rhs |= *this);
    return std::move(*this |= rhs);
}

BigInt BigInt::operator^(const BigInt &rhs) const &
{
    if (rhs.chunks.size() > chunks.size())
    {
        auto res = rhs;
        res ^= *this;
        return res;
    }
    auto res = *this;
    res ^= rhs;
    return res;
}

BigInt BigInt::operator^(BigInt &&rhs) const & { return std::move(rhs ^= *this); }
BigInt BigInt::operator^(const BigInt &rhs) && { return std::move(*this ^= rhs); }

BigInt BigInt::operator^(BigInt &&rhs) &&
{
    if (rhs.chunks.capacity() > chunks.capacity())
        return std::move(rhs ^= *this);
    return std::move(*this ^= rhs);
}

BigInt BigInt::operator<<(const std::int64_t n) const &
{
    auto res = *this;
    res <<= n;
    return res;
}

BigInt BigInt::operator<<(const std::int64_t n) && { return std::move(*this <<= n); }

BigInt BigInt::operator>>(const std::int64_t n) const &
{
    auto res = *this;
    res >>= n;
    return res;
}

BigInt BigInt::operator>>(const std::int64_t n) && { return std::move(*this >>= n); }

BigInt::operator bool() const { return chunks.size(); }

bool BigInt::operator==(const BigInt &rhs) const
{
    return this == &rhs ||
           (isNeg == rhs.isNeg && chunks == rhs.chunks);
}

static std::strong_ordering cmp(const BigInt &diff)
{
    return diff.chunks.size() == 0 ? std::strong_ordering::equal
           : diff.isNeg            ? std::strong_ordering::less
                                   : std::strong_ordering::greater;
}

std::strong_ordering BigInt::operator<=>(const BigInt &rhs) const & { return cmp(*this - rhs); }
std::strong_ordering BigInt::operator<=>(BigInt &&rhs) const & { return cmp(*this - std::move(rhs)); }
std::strong_ordering BigInt::operator<=>(const BigInt &rhs) && { return cmp(std::move(*this) - rhs); }
std::strong_ordering BigInt::operator<=>(BigInt &&rhs) && { return cmp(std::move(*this) - std::move(rhs)); }

void BigInt::negate()
{
    if (*this == Zero())
        return;
    isNeg = !isNeg;
}

void BigInt::invert()
{
    if (isNeg)
        subChunk(chunks, 0, 1);
    else
    {
        chunks.resize(chunks.size() + 1);
        addChunk(chunks, 0, 1);
    }
    isNeg = !isNeg;
    normalize(*this);
}

std::int64_t BigInt::toInteger() const
{
    std::int64_t res = 0;
    bool borrow = true;
    for (std::size_t i = 0; i < 2; ++i)
    {
        auto chunk = i < chunks.size() ? chunks[i] : 0;
        if (isNeg)
        {
            if (borrow)
                borrow = --chunk == static_cast<std::uint32_t>(-1);
            chunk = ~chunk;
        }
        res |= static_cast<std::uint64_t>(chunk) << i * 32;
    }
    return res;
}

template <typename T>
T floatConvert(const BigInt &big)
{
    T res = 0.0;
    const auto n = sizeof(T) / 4 + 1;
    constexpr T chunkMag = 4294967296.0;
    for (auto i = big.chunks.size(), j = n; i-- && j--;)
    {
        res *= chunkMag;
        res += big.chunks[i];
    }
    if (big.chunks.size() > n)
        res *= static_cast<T>(std::pow(chunkMag, big.chunks.size() - n));
    if (big.isNeg)
        res = -res;
    return res;
}

float BigInt::toFloat() const { return floatConvert<float>(*this); }
double BigInt::toDouble() const { return floatConvert<double>(*this); }
long double BigInt::toLongDouble() const { return floatConvert<long double>(*this); }

std::string BigInt::toString() const & { return BigInt(*this).toString(); }

std::string BigInt::toString() &&
{
    if (*this == Zero())
        return "0";
    std::string res = isNeg ? "-" : "";
    std::vector<std::string> digits;
    while (*this)
    {
        auto [q, r] = divmod(*this, TenQuintillion());
        std::uint64_t val = 0;
        if (r.chunks.size() > 1)
            val |= r.chunks[1];
        val <<= 32;
        if (r.chunks.size() > 0)
            val |= r.chunks[0];
        digits.push_back(std::to_string(val));
        *this = std::move(q);
    }
    auto first = true;
    for (std::size_t i = digits.size(); i--;)
    {
        if (!first && digits[i].size() < 19)
            res += std::string(19 - digits[i].size(), '0');
        res += digits[i];
        first = false;
    }
    return res;
}

std::string BigInt::toHex() const
{
    if (*this == Zero())
        return "0x0";
    std::string res = isNeg ? "-0x" : "0x";
    std::vector<std::string> hexChunks(chunks.size(), std::string(8, '0'));
    for (std::size_t i = 0; i < chunks.size(); ++i)
    {
        auto chunk = chunks[i];
        auto &hexChunk = hexChunks[i];
        for (std::size_t j = hexChunk.size(); j-- && chunk;)
        {
            auto off = chunk % 16;
            chunk /= 16;
            hexChunk[j] = static_cast<char>(off < 10 ? '0' + off
                                                     : 'a' + off - 10);
        }
    }
    auto &lastHexChunk = hexChunks.back();
    lastHexChunk = lastHexChunk.substr(lastHexChunk.find_first_not_of('0'));
    for (std::size_t i = hexChunks.size(); i--;)
    {
        res += hexChunks[i];
    }
    return res;
}

BigInt BigInt::fromString(std::string_view str)
{
    constexpr auto exceptionMsg = "BigInt fromString has invalid argument";
    bool strIsNeg = str.size() && str[0] == '-';
    if (strIsNeg)
        str.remove_prefix(1);
    if (str.size() == 0)
        throw std::invalid_argument(exceptionMsg);
    BigInt res;
    while (str.size())
    {
        res *= TenQuintillion();
        auto sub = str.substr(0, str.size() % 19 == 0 ? 19 : str.size() % 19);
        std::uint64_t tmp;
        auto fcRes = std::from_chars(sub.data(), sub.data() + sub.size(), tmp);
        if (fcRes.ec != std::errc{} || fcRes.ptr != sub.data() + sub.size())
            throw std::invalid_argument(exceptionMsg);
        res.chunks.resize(std::max<std::size_t>(res.chunks.size() + 1, 3));
        if (tmp)
            addChunk(res.chunks, 0, static_cast<std::uint32_t>(tmp));
        if (tmp >> 32)
            addChunk(res.chunks, 1, static_cast<std::uint32_t>(tmp >> 32));
        normalize(res);
        str.remove_prefix(sub.size());
    }
    if (strIsNeg)
        res.negate();
    return res;
}

BigInt BigInt::fromHex(std::string_view str)
{
    constexpr auto exceptionMsg = "BigInt fromHex has invalid argument";
    bool strIsNeg = str.substr(0, 3) == "-0x"  ? true
                    : str.substr(0, 2) == "0x" ? false
                                               : throw std::invalid_argument(exceptionMsg);
    str.remove_prefix(strIsNeg ? 3 : 2);
    if (str.size() == 0)
        throw std::invalid_argument(exceptionMsg);
    BigInt res;
    res.chunks.resize(ceilDiv(str.size(), 8));
    for (auto &chunk : res.chunks)
    {
        auto sub = str.substr(str.size() > 8 ? str.size() - 8 : 0);
        str.remove_suffix(sub.size());
        auto fcRes = std::from_chars(sub.data(), sub.data() + sub.size(), chunk, 16);
        if (fcRes.ec != std::errc{} || fcRes.ptr != sub.data() + sub.size())
            throw std::invalid_argument(exceptionMsg);
    }
    normalize(res);
    if (strIsNeg)
        res.negate();
    return res;
}

static int mostSigBit(std::uint32_t x)
{
    if (x == 0)
        return 0;
    int acc = 1;
    for (int s = 16; s > 0; s /= 2)
    {
        if (x >> s)
        {
            x >>= s;
            acc += s;
        }
    }
    return acc;
}

static bool divmodMulSub(BigInt &u, const BigInt &v, const std::size_t j, const std::uint32_t qhat)
{
    bool hasBorrow = false;
    for (std::size_t i = 0; i < v.chunks.size(); ++i)
    {
        auto x = static_cast<std::uint64_t>(v.chunks[i]) * qhat;
        if (x && subChunk(u.chunks, i + j, static_cast<std::uint32_t>(x)))
            hasBorrow = true;
        x >>= 32;
        if (x && subChunk(u.chunks, i + j + 1, static_cast<std::uint32_t>(x)))
            hasBorrow = true;
    }
    return hasBorrow;
}

static bool divmodAddBack(BigInt &u, const BigInt &v, std::size_t j)
{
    bool hasCarry = false;
    for (std::size_t i = 0; i < v.chunks.size(); ++i)
    {
        if (v.chunks[i] && addChunk(u.chunks, i + j, v.chunks[i]))
            hasCarry = true;
    }
    return hasCarry;
}

DivModRes BigInt::divmod(const BigInt &lhs, const BigInt &rhs) { return divmod(BigInt(lhs), BigInt(rhs)); }
DivModRes BigInt::divmod(const BigInt &lhs, BigInt &&rhs) { return divmod(BigInt(lhs), std::move(rhs)); }
DivModRes BigInt::divmod(BigInt &&lhs, const BigInt &rhs) { return divmod(std::move(lhs), BigInt(rhs)); }

DivModRes BigInt::divmod(BigInt &&lhs, BigInt &&rhs)
{
    if (rhs == Zero())
        throw std::invalid_argument("BigInt divmod rhs is zero");
    DivModRes res{{}, std::move(lhs)};
    res.q.isNeg = res.r.isNeg != rhs.isNeg;
    const auto d = 32 - mostSigBit(rhs.chunks.back());
    const auto v = std::move(rhs <<= d);
    res.r <<= d;
    const auto v1 = v.chunks.size() >= 1 ? v.chunks[v.chunks.size() - 1] : 0;
    const auto v2 = v.chunks.size() >= 2 ? v.chunks[v.chunks.size() - 2] : 0;
    const auto n = v.chunks.size();
    if (res.r.chunks.size() + 1 > n)
        res.q.chunks.resize(res.r.chunks.size() + 1 - n);
    for (std::size_t j = res.q.chunks.size(); j--;)
    {
        std::uint64_t uu = res.r.chunks[j + n - 1];
        if (j + n < res.r.chunks.size())
            uu |= static_cast<std::uint64_t>(res.r.chunks[j + n]) << 32;
        std::uint64_t qhat = uu / v1;
        std::uint64_t rhat = uu % v1;
        auto u2 = j + n >= 2 ? res.r.chunks[j + n - 2] : 0;
        while (qhat >> 32 || qhat * v2 > (rhat << 32 | u2))
        {
            --qhat;
            rhat += v1;
            if (rhat >> 32)
                break;
        }
        if (qhat == 0)
            continue;
        if (divmodMulSub(res.r, v, j, static_cast<std::uint32_t>(qhat)))
            do
                --qhat;
            while (!divmodAddBack(res.r, v, j));
        normalize(res.r);
        res.q.chunks[j] = static_cast<std::uint32_t>(qhat);
    }
    normalize(res.q);
    res.r >>= d;
    return res;
}

BigInt BigInt::pow(const BigInt &base, std::int64_t exp)
{
    if (exp < 0)
        throw std::invalid_argument("BigInt pow has negative exponent");
    if (exp == 0)
        return One();
    auto x = base;
    auto y = One();
    while (exp > 1)
    {
        if (exp % 2)
        {
            y *= x;
            --exp;
        }
        x *= x;
        exp /= 2;
    }
    return x * y;
}
