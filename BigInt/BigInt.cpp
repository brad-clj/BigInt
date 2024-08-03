#include "BigInt.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

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
    static const auto val = BigInt::fromHex("0x8ac7230489e80000");
    return val;
}

template <typename N, typename D>
auto ceilDiv(const N n, const D d)
{
    return n / d + (n % d ? 1 : 0);
}

static bool addChunk(std::vector<uint32_t> &chunks, size_t i, const uint32_t val)
{
    auto hasCarry = (chunks[i++] += val) < val;
    while (hasCarry && i < chunks.size())
    {
        hasCarry = ++chunks[i++] == 0;
    }
    return hasCarry;
}

static void addChunkFast(std::vector<uint32_t> &chunks, size_t i, const uint32_t val)
{
    auto hasCarry = (chunks[i++] += val) < val;
    while (hasCarry)
    {
        hasCarry = ++chunks[i++] == 0;
    }
}

static bool subChunk(std::vector<uint32_t> &chunks, size_t i, const uint32_t val)
{
    auto prev = chunks[i];
    auto hasBorrow = (chunks[i++] -= val) > prev;
    while (hasBorrow && i < chunks.size())
    {
        hasBorrow = --chunks[i++] == static_cast<uint32_t>(-1);
    }
    return hasBorrow;
}

static void subChunkFast(std::vector<uint32_t> &chunks, size_t i, const uint32_t val)
{
    auto prev = chunks[i];
    auto hasBorrow = (chunks[i++] -= val) > prev;
    while (hasBorrow)
    {
        hasBorrow = --chunks[i++] == static_cast<uint32_t>(-1);
    }
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
    for (size_t i = 0; i < other.chunks.size(); ++i)
    {
        if (other.chunks[i])
            addChunkFast(acc.chunks, i, other.chunks[i]);
    }
    normalize(acc);
}

static void sub(BigInt &acc, const BigInt &other)
{
    if (acc.chunks.size() <= other.chunks.size())
        acc.chunks.resize(std::max(acc.chunks.size(), other.chunks.size()));
    bool hasBorrow = false;
    for (size_t i = 0; i < other.chunks.size(); ++i)
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
        addChunkFast(acc.chunks, 0, 1);
        acc.negate();
    }
    normalize(acc);
}

static void subFast(BigInt &acc, const BigInt &other)
{
    for (size_t i = 0; i < other.chunks.size(); ++i)
    {
        if (other.chunks[i])
            subChunkFast(acc.chunks, i, other.chunks[i]);
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
                borrow = --chunk == static_cast<uint32_t>(-1);
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
        chunks.push_back(static_cast<uint32_t>(num));
        num >>= 32;
    }
}

BigInt::BigInt(const double num)
{
    if (!std::isnormal(num))
        return;
    int exp;
    auto fr = std::frexp(std::trunc(num), &exp);
    if (fr < 0.0)
    {
        fr = -fr;
        isNeg = true;
    }
    chunks.resize(ceilDiv(exp, 32));
    for (size_t i = chunks.size(); i--;)
    {
        auto &chunk = chunks[i];
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
    {
        if (chunks.size() > rhs.chunks.size())
            subFast(*this, rhs);
        else
            sub(*this, rhs);
    }
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
    {
        if (chunks.size() > rhs.chunks.size())
            subFast(*this, rhs);
        else
            sub(*this, rhs);
    }
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

BigInt &BigInt::operator*=(const BigInt &rhs)
{
    return *this = *this * rhs;
}

BigInt &BigInt::operator/=(const BigInt &rhs)
{
    return *this = std::move(*this) / rhs;
}

BigInt &BigInt::operator/=(BigInt &&rhs)
{
    return *this = std::move(*this) / std::move(rhs);
}

BigInt &BigInt::operator%=(const BigInt &rhs)
{
    return *this = std::move(*this) % rhs;
}

BigInt &BigInt::operator%=(BigInt &&rhs)
{
    return *this = std::move(*this) % std::move(rhs);
}

static void bitwise(BigInt &lhs, const BigInt &rhs, const std::function<void(uint32_t &, uint32_t)> &fn)
{
    uint32_t x = lhs.isNeg ? static_cast<uint32_t>(-1) : 0;
    fn(x, rhs.isNeg ? static_cast<uint32_t>(-1) : 0);
    bool resIsNeg = static_cast<bool>(x);
    lhs.chunks.resize(std::max(lhs.chunks.size(), rhs.chunks.size()) + (resIsNeg ? 1 : 0));
    bool lhsBorrow = lhs.isNeg;
    bool rhsBorrow = rhs.isNeg;
    bool resBorrow = resIsNeg;
    for (size_t i = 0; i < lhs.chunks.size(); ++i)
    {
        auto &a = lhs.chunks[i];
        if (lhs.isNeg)
        {
            if (lhsBorrow)
                lhsBorrow = --a == static_cast<uint32_t>(-1);
            a = ~a;
        }
        auto b = i < rhs.chunks.size() ? rhs.chunks[i] : 0;
        if (rhs.isNeg)
        {
            if (rhsBorrow)
                rhsBorrow = --b == static_cast<uint32_t>(-1);
            b = ~b;
        }
        fn(a, b);
        if (resIsNeg)
        {
            if (resBorrow)
                resBorrow = --a == static_cast<uint32_t>(-1);
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
            [](uint32_t &a, uint32_t b)
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
            [](uint32_t &a, uint32_t b)
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
            [](uint32_t &a, uint32_t b)
            { a ^= b; });
    return *this;
}

BigInt &BigInt::operator^=(BigInt &&rhs)
{
    if (rhs.chunks.capacity() > chunks.capacity())
        std::swap(*this, rhs);
    return *this ^= rhs;
}

BigInt &BigInt::operator<<=(const size_t n)
{
    if (n == 0)
        return *this;
    const auto off = n / 32;
    const auto s = n % 32;
    chunks.resize(chunks.size() + ceilDiv(n, 32));
    for (auto i = chunks.size(); i--;)
    {
        uint32_t x = 0;
        if (i >= off)
            x = chunks[i - off] << s;
        if (s && i >= off + 1)
            x |= chunks[i - (off + 1)] >> (32 - s);
        chunks[i] = x;
    }
    normalize(*this);
    return *this;
}

BigInt &BigInt::operator>>=(const size_t n)
{
    if (n == 0)
        return *this;
    if (n >= chunks.size() * 32)
    {
        chunks.clear();
        if (isNeg)
            chunks.push_back(1);
        return *this;
    }
    if (isNeg)
        subChunkFast(chunks, 0, 1);
    const auto off = n / 32;
    const auto s = n % 32;
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        uint32_t x = 0;
        if (i + off < chunks.size())
            x = chunks[i + off] >> s;
        if (s && i + off + 1 < chunks.size())
            x |= chunks[i + off + 1] << (32 - s);
        chunks[i] = x;
    }
    if (isNeg)
        addChunkFast(chunks, 0, 1);
    normalize(*this);
    return *this;
}

BigInt &BigInt::operator++()
{
    return *this += One();
}

BigInt &BigInt::operator--()
{
    return *this -= One();
}

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

BigInt &&BigInt::operator-() &&
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

BigInt &&BigInt::operator+(BigInt &&rhs) const &
{
    return std::move(rhs += *this);
}

BigInt &&BigInt::operator+(const BigInt &rhs) &&
{
    return std::move(*this += rhs);
}

BigInt &&BigInt::operator+(BigInt &&rhs) &&
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

BigInt &&BigInt::operator-(BigInt &&rhs) const &
{
    if (this == &rhs)
        return std::move(rhs = Zero());
    rhs.negate();
    return std::move(rhs += *this);
}

BigInt &&BigInt::operator-(const BigInt &rhs) &&
{
    return std::move(*this -= rhs);
}

BigInt &&BigInt::operator-(BigInt &&rhs) &&
{
    if (rhs.chunks.capacity() > chunks.capacity())
        return *this - std::move(rhs);
    return std::move(*this -= rhs);
}

static BigInt mul(const BigInt &lhs, const BigInt &rhs)
{
    BigInt res;
    res.chunks.resize(lhs.chunks.size() + rhs.chunks.size() + 1);
    for (size_t i = 0; i < lhs.chunks.size(); ++i)
    {
        for (size_t j = 0; j < rhs.chunks.size(); ++j)
        {
            auto prod = static_cast<uint64_t>(lhs.chunks[i]) * rhs.chunks[j];
            if (prod)
                addChunkFast(res.chunks, i + j, static_cast<uint32_t>(prod));
            if (prod >> 32)
                addChunkFast(res.chunks, i + j + 1, static_cast<uint32_t>(prod >> 32));
        }
    }
    return res;
}

struct Toom2Split
{
    BigInt low, high;

    Toom2Split(const BigInt &big, const size_t sz)
    {
        size_t i = 0;
        low.chunks.resize(std::min(big.chunks.size(), sz));
        for (; i < big.chunks.size() && i < sz; ++i)
        {
            low.chunks[i] = big.chunks[i];
        }
        high.chunks.resize(big.chunks.size() - i);
        for (; i < big.chunks.size(); ++i)
        {
            high.chunks[i - sz] = big.chunks[i];
        }
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
    for (size_t i = 0; i < r.size(); ++i)
    {
        for (size_t j = 0; j < r[i].chunks.size(); ++j)
        {
            if (r[i].chunks[j])
                addChunkFast(res.chunks, sz * i + j, r[i].chunks[j]);
        }
    }
    return res;
}

struct Toom3Mat
{
    BigInt zero, one, negone, negtwo, inf;

    Toom3Mat(const BigInt &big, const size_t sz)
    {
        BigInt b0, b1, b2;
        size_t i = 0;
        b0.chunks.resize(std::min(big.chunks.size(), sz));
        for (; i < big.chunks.size() && i < sz; ++i)
        {
            b0.chunks[i] = big.chunks[i];
        }
        b1.chunks.resize(std::min(big.chunks.size() - i, sz * 2 - i));
        for (; i < big.chunks.size() && i < sz * 2; ++i)
        {
            b1.chunks[i - sz] = big.chunks[i];
        }
        b2.chunks.resize(big.chunks.size() - i);
        for (; i < big.chunks.size(); ++i)
        {
            b2.chunks[i - sz * 2] = big.chunks[i];
        }
        auto tmp = b0 + b2;
        zero = b0;
        one = tmp + b1;
        negone = std::move(tmp) - std::move(b1);
        negtwo = ((negone + b2) << 1) - std::move(b0);
        inf = std::move(b2);
    }
};

static BigInt &&div2(BigInt &&big)
{
    static const BigInt negOne(-1);
    if (big == negOne)
        big = Zero();
    else
        big >>= 1;
    return std::move(big);
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
    for (size_t i = 0; i < r.size(); ++i)
    {
        for (size_t j = 0; j < r[i].chunks.size(); ++j)
        {
            if (r[i].chunks[j])
                addChunkFast(res.chunks, sz * i + j, r[i].chunks[j]);
        }
    }
    return res;
}

static constexpr size_t Toom2Thresh = 550;
static constexpr size_t Toom3Thresh = 2200;

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

BigInt BigInt::operator/(const BigInt &rhs) const &
{
    return divmod(*this, rhs).q;
}

BigInt BigInt::operator/(BigInt &&rhs) const &
{
    return divmod(*this, std::move(rhs)).q;
}

BigInt BigInt::operator/(const BigInt &rhs) &&
{
    return divmod(std::move(*this), rhs).q;
}

BigInt BigInt::operator/(BigInt &&rhs) &&
{
    return divmod(std::move(*this), std::move(rhs)).q;
}

BigInt BigInt::operator%(const BigInt &rhs) const &
{
    return divmod(*this, rhs).r;
}

BigInt BigInt::operator%(BigInt &&rhs) const &
{
    return divmod(*this, std::move(rhs)).r;
}

BigInt BigInt::operator%(const BigInt &rhs) &&
{
    return divmod(std::move(*this), rhs).r;
}

BigInt BigInt::operator%(BigInt &&rhs) &&
{
    return divmod(std::move(*this), std::move(rhs)).r;
}

BigInt BigInt::operator~() const &
{
    auto res = *this;
    res.invert();
    return res;
}

BigInt &&BigInt::operator~() &&
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

BigInt &&BigInt::operator&(BigInt &&rhs) const &
{
    return std::move(rhs &= *this);
}

BigInt &&BigInt::operator&(const BigInt &rhs) &&
{
    return std::move(*this &= rhs);
}

BigInt &&BigInt::operator&(BigInt &&rhs) &&
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

BigInt &&BigInt::operator|(BigInt &&rhs) const &
{
    return std::move(rhs |= *this);
}

BigInt &&BigInt::operator|(const BigInt &rhs) &&
{
    return std::move(*this |= rhs);
}

BigInt &&BigInt::operator|(BigInt &&rhs) &&
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

BigInt &&BigInt::operator^(BigInt &&rhs) const &
{
    return std::move(rhs ^= *this);
}

BigInt &&BigInt::operator^(const BigInt &rhs) &&
{
    return std::move(*this ^= rhs);
}

BigInt &&BigInt::operator^(BigInt &&rhs) &&
{
    if (rhs.chunks.capacity() > chunks.capacity())
        return std::move(rhs ^= *this);
    return std::move(*this ^= rhs);
}

BigInt BigInt::operator<<(const size_t n) const &
{
    auto res = *this;
    res <<= n;
    return res;
}

BigInt &&BigInt::operator<<(const size_t n) &&
{
    return std::move(*this <<= n);
}

BigInt BigInt::operator>>(const size_t n) const &
{
    auto res = *this;
    res >>= n;
    return res;
}

BigInt &&BigInt::operator>>(const size_t n) &&
{
    return std::move(*this >>= n);
}

BigInt::operator bool() const
{
    return chunks.size();
}

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

std::strong_ordering BigInt::operator<=>(const BigInt &rhs) const &
{
    return cmp(*this - rhs);
}

std::strong_ordering BigInt::operator<=>(BigInt &&rhs) const &
{
    return cmp(*this - std::move(rhs));
}

std::strong_ordering BigInt::operator<=>(const BigInt &rhs) &&
{
    return cmp(std::move(*this) - rhs);
}

std::strong_ordering BigInt::operator<=>(BigInt &&rhs) &&
{
    return cmp(std::move(*this) - std::move(rhs));
}

void BigInt::negate()
{
    if (*this == Zero())
        return;
    isNeg = !isNeg;
}

void BigInt::invert()
{
    if (isNeg)
        subChunkFast(chunks, 0, 1);
    else
    {
        chunks.resize(chunks.size() + 1);
        addChunkFast(chunks, 0, 1);
    }
    isNeg = !isNeg;
    normalize(*this);
}

int64_t BigInt::toInteger() const
{
    int64_t res = 0;
    bool borrow = true;
    for (size_t i = 0; i < 2; ++i)
    {
        auto chunk = i < chunks.size() ? chunks[i] : 0;
        if (isNeg)
        {
            if (borrow)
                borrow = --chunk == static_cast<uint32_t>(-1);
            chunk = ~chunk;
        }
        res |= static_cast<uint64_t>(chunk) << i * 32;
    }
    return res;
}

double BigInt::toDouble() const
{
    double res = 0.0;
    int n = 3;
    for (size_t i = chunks.size(); i-- && n--;)
    {
        res *= std::pow(2, 32);
        res += chunks[i];
    }
    if (chunks.size() > 3)
        res *= std::pow(2, 32 * (chunks.size() - 3));
    if (isNeg)
        res = -res;
    return res;
}

std::string BigInt::toString() const &
{
    return BigInt(*this).toString();
}

std::string BigInt::toString() &&
{
    if (*this == Zero())
        return "0";
    std::string res = isNeg ? "-" : "";
    std::vector<std::string> digits;
    while (*this)
    {
        auto [q, r] = divmod(*this, TenQuintillion());
        uint64_t val = 0;
        if (r.chunks.size() > 1)
            val |= r.chunks[1];
        val <<= 32;
        if (r.chunks.size() > 0)
            val |= r.chunks[0];
        digits.push_back(std::to_string(val));
        *this = std::move(q);
    }
    auto first = true;
    for (size_t i = digits.size(); i--;)
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
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        auto chunk = chunks[i];
        auto &hexChunk = hexChunks[i];
        for (size_t j = hexChunk.size(); j-- && chunk;)
        {
            auto off = chunk % 16;
            chunk /= 16;
            hexChunk[j] = static_cast<char>(off < 10 ? '0' + off
                                                     : 'a' + off - 10);
        }
    }
    auto &lastHexChunk = hexChunks.back();
    lastHexChunk = lastHexChunk.substr(lastHexChunk.find_first_not_of('0'));
    for (size_t i = hexChunks.size(); i--;)
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
        uint64_t tmp;
        auto fcRes = std::from_chars(sub.data(), sub.data() + sub.size(), tmp);
        if (fcRes.ec != std::errc{} || fcRes.ptr != sub.data() + sub.size())
            throw std::invalid_argument(exceptionMsg);
        res.chunks.resize(std::max<size_t>(res.chunks.size() + 1, 3));
        if (tmp)
            addChunkFast(res.chunks, 0, static_cast<uint32_t>(tmp));
        if (tmp >> 32)
            addChunkFast(res.chunks, 1, static_cast<uint32_t>(tmp >> 32));
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

static int bitCount(uint32_t x)
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

static bool divmodMulSub(BigInt &r, uint64_t x, const BigInt &d, size_t i)
{
    bool hasBorrow = false;
    for (; x; x >>= 32, ++i)
    {
        auto y = static_cast<uint32_t>(x);
        for (size_t j = 0; j < d.chunks.size(); ++j)
        {
            uint64_t z = d.chunks[j];
            z *= y;
            if (z && subChunk(r.chunks, i + j, static_cast<uint32_t>(z)))
                hasBorrow = true;
            if (z >> 32 && subChunk(r.chunks, i + j + 1, static_cast<uint32_t>(z >> 32)))
                hasBorrow = true;
        }
    }
    normalize(r);
    return hasBorrow;
}

static bool divmodAddBack(BigInt &r, const BigInt &d, const size_t i)
{
    bool hasCarry = false;
    for (size_t j = 0; j < d.chunks.size(); ++j)
    {
        if (d.chunks[j] && addChunk(r.chunks, i + j, d.chunks[j]))
            hasCarry = true;
    }
    normalize(r);
    return hasCarry;
}

DivModRes BigInt::divmod(const BigInt &lhs, const BigInt &rhs)
{
    return divmod(BigInt(lhs), BigInt(rhs));
}

DivModRes BigInt::divmod(const BigInt &lhs, BigInt &&rhs)
{
    return divmod(BigInt(lhs), std::move(rhs));
}

DivModRes BigInt::divmod(BigInt &&lhs, const BigInt &rhs)
{
    return divmod(std::move(lhs), BigInt(rhs));
}

DivModRes BigInt::divmod(BigInt &&lhs, BigInt &&rhs)
{
    DivModRes res{{}, std::move(lhs)};
    res.q.isNeg = res.r.isNeg != rhs.isNeg;
    auto d = std::move(rhs);
    const int s = 32 - bitCount(d.chunks.back());
    res.r <<= s;
    d <<= s;
    if (res.r.chunks.size() + 1 > d.chunks.size())
        res.q.chunks.resize(res.r.chunks.size() + 1 - d.chunks.size());
    const auto x = d.chunks.back();
    for (size_t i = res.r.chunks.size(); i-- >= d.chunks.size();)
    {
        auto y = static_cast<uint64_t>(res.r.chunks[i]);
        y |= i + 1 < res.r.chunks.size() ? static_cast<uint64_t>(res.r.chunks[i + 1]) << 32 : 0;
        auto z = y / x;
        size_t j = i - d.chunks.size() + 1;
        if (divmodMulSub(res.r, z, d, j))
            do
            {
                --z;
            } while (!divmodAddBack(res.r, d, j));
        if (z)
            addChunkFast(res.q.chunks, j, static_cast<uint32_t>(z));
        if (z >> 32)
            addChunkFast(res.q.chunks, j + 1, static_cast<uint32_t>(z >> 32));
    }
    normalize(res.q);
    normalize(res.r);
    res.r >>= s;
    return res;
}
