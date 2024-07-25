#include "BigInt.h"
#include <algorithm>
#include <charconv>
#include <functional>
#include <stdexcept>
#include <utility>

static const BigInt &Zero()
{
    static const BigInt val(0);
    return val;
}

static const BigInt &NegOne()
{
    static const BigInt val(-1);
    return val;
}

static const BigInt &One()
{
    static const BigInt val(1);
    return val;
}

static const BigInt &Three()
{
    static const BigInt val(3);
    return val;
}

static const BigInt &OneExa()
{
    static const BigInt val(1'000'000'000'000'000'000);
    return val;
}

BigInt::BigInt() {}

BigInt::BigInt(int64_t num)
{
    auto tmp = num;
    size_t count = 0;
    while (tmp != 0 && tmp != -1)
    {
        ++count;
        tmp >>= 32;
    }
    data.reserve(count);
    negative = num < 0;
    while (num != 0 && num != -1)
    {
        data.push_back(static_cast<uint32_t>(num));
        num >>= 32;
    }
}

BigInt::BigInt(std::string_view str)
{
    auto neg = str.size() && str[0] == '-';
    if (neg)
        str = str.substr(1);
    if (str.size() == 0)
        throw std::invalid_argument("BigInt string_view ctor has invalid argument");
    auto sub = str.substr(0, str.size() % 18);
    int64_t tmp;
    if (sub.size())
    {
        auto res = std::from_chars(sub.data(), sub.data() + sub.size(), tmp);
        if (res.ec != std::errc{} || res.ptr - sub.data() != sub.size())
            throw std::invalid_argument("BigInt string_view ctor has invalid argument");
        *this += tmp;
        str = str.substr(str.size() % 18);
    }
    while (str.size())
    {
        *this *= OneExa();
        sub = str.substr(0, 18);
        auto res = std::from_chars(sub.data(), sub.data() + sub.size(), tmp);
        if (res.ec != std::errc{} || res.ptr - sub.data() != sub.size())
            throw std::invalid_argument("BigInt string_view ctor has invalid argument");
        *this += tmp;
        str = str.substr(18);
    }
    if (neg)
        negate();
}

BigInt &BigInt::operator+=(const BigInt &rhs)
{
    if (this == &rhs)
        return *this += BigInt(rhs);
    size_t i = 0;
    data.reserve(std::max(data.size(), rhs.data.size()) + 1);
    auto lim = data.size();
    while (i < rhs.data.size())
    {
        addChunk(i++, rhs.data[i]);
    }
    if (rhs.negative)
    {
        while (i < lim)
        {
            addChunk(i++, static_cast<uint32_t>(-1));
        }
        if (i < data.size())
            data.pop_back();
        else
        {
            if (negative)
                addChunk(i, static_cast<uint32_t>(-1));
            negative = true;
        }
    }
    normalize();
    return *this;
}

BigInt &BigInt::operator+=(BigInt &&rhs)
{
    if (rhs.data.capacity() > data.capacity())
        std::swap(*this, rhs);
    return *this += rhs;
}

BigInt &BigInt::operator-=(const BigInt &rhs)
{
    if (this == &rhs)
        return *this = Zero();
    size_t i = 0;
    data.reserve(std::max(data.size(), rhs.data.size()) + 1);
    auto lim = data.size();
    while (i < rhs.data.size())
    {
        subChunk(i++, rhs.data[i]);
    }
    if (rhs.negative)
    {
        while (i < lim)
        {
            subChunk(i++, static_cast<uint32_t>(-1));
        }
        if (i < data.size())
            data.pop_back();
        else
        {
            if (!negative)
                subChunk(i, static_cast<uint32_t>(-1));
            negative = false;
        }
    }
    normalize();
    return *this;
}

BigInt &BigInt::operator-=(BigInt &&rhs)
{
    if (rhs.data.capacity() > data.capacity())
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
    return *this = *this / rhs;
}

BigInt &BigInt::operator%=(const BigInt &rhs)
{
    return *this = *this % rhs;
}

static void bitwise(BigInt &lhs, const BigInt &rhs, std::function<void(uint32_t &, uint32_t)> fn)
{
    auto lim = std::max(lhs.data.size(), rhs.data.size());
    for (size_t i = 0; i < lim; ++i)
    {
        if (i == lhs.data.size())
            lhs.data.push_back(lhs.negative ? static_cast<uint32_t>(-1) : 0);
        auto &a = lhs.data[i];
        auto b = i < rhs.data.size() ? rhs.data[i]
                 : rhs.negative      ? static_cast<uint32_t>(-1)
                                     : 0;
        fn(a, b);
    }
    uint32_t x = lhs.negative ? static_cast<uint32_t>(-1) : 0;
    fn(x, rhs.negative ? static_cast<uint32_t>(-1) : 0);
    lhs.negative = static_cast<bool>(x);
    lhs.normalize();
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
    if (rhs.data.capacity() > data.capacity())
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
    if (rhs.data.capacity() > data.capacity())
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
    if (rhs.data.capacity() > data.capacity())
        std::swap(*this, rhs);
    return *this ^= rhs;
}

template <typename N, typename D>
auto ceilDiv(const N n, const D d)
{
    return n / d + (n % d ? 1 : 0);
}

BigInt &BigInt::operator<<=(const size_t n)
{
    if (n == 0)
        return *this;
    const auto oldSz = data.size();
    const auto off = n / 32;
    const auto s = n % 32;
    data.resize(data.size() + ceilDiv(n, 32));
    for (auto i = data.size(); i--;)
    {
        auto x = i < off           ? 0
                 : i - off < oldSz ? data[i - off] << s
                 : negative        ? static_cast<uint32_t>(-1) << s
                                   : 0;
        if (s)
            x |= i < off + 1 ? 0 : data[i - off - 1] >> (32 - s);
        data[i] = x;
    }
    normalize();
    return *this;
}

BigInt &BigInt::operator>>=(const size_t n)
{
    if (n == 0)
        return *this;
    const auto off = n / 32;
    const auto s = n % 32;
    for (size_t i = 0; i < data.size(); ++i)
    {
        auto x = i + off < data.size() ? data[i + off] >> s
                 : negative            ? static_cast<uint32_t>(-1) >> s
                                       : 0;
        if (s)
            x |= i + off + 1 < data.size() ? data[i + off + 1] << (32 - s)
                 : negative                ? static_cast<uint32_t>(-1) << (32 - s)
                                           : 0;
        data[i] = x;
    }
    normalize();
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

BigInt BigInt::operator-() &&
{
    negate();
    return std::move(*this);
}

BigInt BigInt::operator+(const BigInt &rhs) const &
{
    if (rhs.data.size() > data.size())
    {
        auto res = rhs;
        return res += *this;
    }
    auto res = *this;
    return res += rhs;
}

BigInt BigInt::operator+(BigInt &&rhs) const &
{
    return std::move(rhs += *this);
}

BigInt BigInt::operator+(const BigInt &rhs) &&
{
    return std::move(*this += rhs);
}

BigInt BigInt::operator+(BigInt &&rhs) &&
{
    if (rhs.data.capacity() > data.capacity())
        return std::move(rhs += *this);
    return std::move(*this += rhs);
}

BigInt BigInt::operator-(const BigInt &rhs) const &
{
    if (rhs.data.size() > data.size())
    {
        auto res = rhs;
        res.negate();
        return res += *this;
    }
    auto res = *this;
    return res -= rhs;
}

BigInt BigInt::operator-(BigInt &&rhs) const &
{
    rhs.negate();
    return std::move(rhs += *this);
}

BigInt BigInt::operator-(const BigInt &rhs) &&
{
    return std::move(*this -= rhs);
}

BigInt BigInt::operator-(BigInt &&rhs) &&
{
    if (rhs.data.capacity() > data.capacity())
        return *this - std::move(rhs);
    return std::move(*this -= rhs);
}

static constexpr size_t Toom2Thresh = 500;
static constexpr size_t Toom3Thresh = 2000;

BigInt BigInt::operator*(const BigInt &rhs) const
{
    auto &lhs = *this;
    if (!lhs.negative && !rhs.negative)
    {
        const auto score = lhs.data.size() * rhs.data.size();
        if (score > Toom3Thresh)
            return toom3(lhs, rhs);
        if (score > Toom2Thresh)
            return toom2(lhs, rhs);
        BigInt res;
        res.data.reserve(lhs.data.size() + rhs.data.size());
        for (size_t i = 0; i < lhs.data.size(); ++i)
        {
            for (size_t j = 0; j < rhs.data.size(); ++j)
            {
                auto prod = static_cast<uint64_t>(lhs.data[i]) * rhs.data[j];
                if (prod)
                    res.addChunk(i + j, static_cast<uint32_t>(prod));
                if (prod >> 32)
                    res.addChunk(i + j + 1, static_cast<uint32_t>(prod >> 32));
            }
        }
        res.normalize();
        return res;
    }
    else
    {
        auto res = lhs.negative && rhs.negative ? -lhs * -rhs
                   : lhs.negative               ? -lhs * rhs
                                                : lhs * -rhs;
        if (lhs.negative != rhs.negative)
            res.negate();
        return res;
    }
}

BigInt BigInt::operator/(const BigInt &rhs) const
{
    return divmod(*this, rhs).q;
}

BigInt BigInt::operator%(const BigInt &rhs) const
{
    return divmod(*this, rhs).r;
}

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
    if (rhs.data.size() > data.size())
    {
        auto res = rhs;
        return res &= *this;
    }
    auto res = *this;
    return res &= rhs;
}

BigInt BigInt::operator&(BigInt &&rhs) const &
{
    return std::move(rhs &= *this);
}

BigInt BigInt::operator&(const BigInt &rhs) &&
{
    return std::move(*this &= rhs);
}

BigInt BigInt::operator&(BigInt &&rhs) &&
{
    if (rhs.data.capacity() > data.capacity())
        return std::move(rhs &= *this);
    return std::move(*this &= rhs);
}

BigInt BigInt::operator|(const BigInt &rhs) const &
{
    if (rhs.data.size() > data.size())
    {
        auto res = rhs;
        return res |= *this;
    }
    auto res = *this;
    return res |= rhs;
}

BigInt BigInt::operator|(BigInt &&rhs) const &
{
    return std::move(rhs |= *this);
}

BigInt BigInt::operator|(const BigInt &rhs) &&
{
    return std::move(*this |= rhs);
}

BigInt BigInt::operator|(BigInt &&rhs) &&
{
    if (rhs.data.capacity() > data.capacity())
        return std::move(rhs |= *this);
    return std::move(*this |= rhs);
}

BigInt BigInt::operator^(const BigInt &rhs) const &
{
    if (rhs.data.size() > data.size())
    {
        auto res = rhs;
        return res ^= *this;
    }
    auto res = *this;
    return res ^= rhs;
}

BigInt BigInt::operator^(BigInt &&rhs) const &
{
    return std::move(rhs ^= *this);
}

BigInt BigInt::operator^(const BigInt &rhs) &&
{
    return std::move(*this ^= rhs);
}

BigInt BigInt::operator^(BigInt &&rhs) &&
{
    if (rhs.data.capacity() > data.capacity())
        return std::move(rhs ^= *this);
    return std::move(*this ^= rhs);
}

BigInt BigInt::operator<<(const size_t n) const &
{
    auto res = *this;
    return res <<= n;
}

BigInt BigInt::operator<<(const size_t n) &&
{
    return std::move(*this <<= n);
}

BigInt BigInt::operator>>(const size_t n) const &
{
    auto res = *this;
    return res >>= n;
}

BigInt BigInt::operator>>(const size_t n) &&
{
    return std::move(*this >>= n);
}

BigInt::operator bool() const
{
    return negative || data.size();
}

bool BigInt::operator==(const BigInt &rhs) const
{
    return negative == rhs.negative && data == rhs.data;
}

std::strong_ordering BigInt::operator<=>(const BigInt &rhs) const
{
    auto diff = *this - rhs;
    return !diff.negative && diff.data.size() == 0 ? std::strong_ordering::equal
           : diff.negative                         ? std::strong_ordering::less
                                                   : std::strong_ordering::greater;
}

std::string BigInt::toString() const
{
    if (*this == Zero())
        return "0";
    std::string res = negative ? "-" : "";
    auto num = negative ? -*this : *this;
    std::vector<std::string> digits;
    while (num)
    {
        auto [q, r] = divmod(num, OneExa());
        uint64_t val = r.data.size() > 1 ? r.data[1] : 0;
        val <<= 32;
        val += r.data.size() > 0 ? r.data[0] : 0;
        digits.push_back(std::to_string(val));
        num = std::move(q);
    }
    auto first = true;
    for (size_t i = digits.size(); i--;)
    {
        if (!first && digits[i].size() < 18)
            res += std::string(18 - digits[i].size(), '0');
        res += digits[i];
        first = false;
    }
    return res;
}

void BigInt::normalize()
{
    while (data.size() && data.back() == (negative ? static_cast<uint32_t>(-1) : 0))
    {
        data.pop_back();
    }
}

void BigInt::negate()
{
    invert();
    addChunk(0, 1);
    normalize();
}

void BigInt::invert()
{
    for (auto &x : data)
    {
        x = ~x;
    }
    negative = !negative;
}

void BigInt::addChunk(size_t i, const uint32_t val)
{
    while (data.size() <= i)
    {
        data.push_back(negative ? static_cast<uint32_t>(-1) : 0);
    }
    auto carry = (data[i++] += val) < val;
    while (carry)
    {
        if (i == data.size())
        {
            if (negative)
            {
                negative = false;
                return;
            }
            data.push_back(0);
        }
        carry = ++data[i++] == 0;
    }
}

void BigInt::subChunk(size_t i, const uint32_t val)
{
    while (data.size() <= i)
    {
        data.push_back(negative ? static_cast<uint32_t>(-1) : 0);
    }
    auto prev = data[i];
    auto borrow = (data[i++] -= val) > prev;
    while (borrow)
    {
        if (i == data.size())
        {
            if (!negative)
            {
                negative = true;
                return;
            }
            data.push_back(static_cast<uint32_t>(-1));
        }
        borrow = --data[i++] == static_cast<uint32_t>(-1);
    }
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

static void divmodMulSub(BigInt &r, uint64_t x, const BigInt &d, size_t i)
{
    for (; x; x >>= 32, ++i)
    {
        auto y = static_cast<uint32_t>(x);
        for (size_t j = 0; j < d.data.size(); ++j)
        {
            uint64_t z = d.data[j];
            z *= y;
            if (z)
                r.subChunk(i + j, static_cast<uint32_t>(z));
            if (z >> 32)
                r.subChunk(i + j + 1, static_cast<uint32_t>(z >> 32));
        }
    }
    r.normalize();
}

static void divmodAddBack(BigInt &r, const BigInt &d, const size_t i)
{
    for (size_t j = 0; j < d.data.size(); ++j)
    {
        r.addChunk(i + j, d.data[j]);
    }
    r.normalize();
}

DivModRes BigInt::divmod(const BigInt &lhs, const BigInt &rhs)
{
    DivModRes res{{}, lhs};
    auto d = rhs;
    if (lhs.negative)
        res.r.negate();
    if (rhs.negative)
        d.negate();
    const int s = 32 - bitCount(d.data.back());
    res.r <<= s;
    d <<= s;
    if (res.r.data.size() + 1 > d.data.size())
        res.q.data.reserve(res.r.data.size() + 1 - d.data.size());
    const auto x = d.data.back();
    for (size_t i = res.r.data.size(); i-- >= d.data.size();)
    {
        auto y = static_cast<uint64_t>(res.r.data[i]);
        y |= i + 1 < res.r.data.size() ? static_cast<uint64_t>(res.r.data[i + 1]) << 32 : 0;
        auto z = y / x;
        size_t j = i - d.data.size() + 1;
        divmodMulSub(res.r, z, d, j);
        while (res.r.negative)
        {
            --z;
            divmodAddBack(res.r, d, j);
        }
        if (z)
            res.q.addChunk(j, static_cast<uint32_t>(z));
        if (z >> 32)
            res.q.addChunk(j + 1, static_cast<uint32_t>(z >> 32));
    }
    res.q.normalize();
    res.r.normalize();
    res.r >>= s;
    if (lhs.negative != rhs.negative)
        res.q.negate();
    if (lhs.negative)
        res.r.negate();
    return res;
}

namespace
{
    struct Toom2Split
    {
        BigInt low, high;

        Toom2Split(const BigInt &big, const size_t sz)
        {
            size_t i = 0;
            size_t bigSz = big.data.size();
            low.data.reserve(std::min(bigSz, sz));
            for (; i < bigSz && i < sz; ++i)
            {
                low.data.push_back(big.data[i]);
            }
            high.data.reserve(bigSz - i);
            for (; i < bigSz; ++i)
            {
                high.data.push_back(big.data[i]);
            }
        }
    };
}

BigInt BigInt::toom2(const BigInt &lhs, const BigInt &rhs)
{
    const auto sz = ceilDiv(std::max(lhs.data.size(), rhs.data.size()), 2);
    Toom2Split p(lhs, sz);
    Toom2Split q(rhs, sz);
    auto high = p.high * q.high;
    auto low = p.low * q.low;
    auto mid = low + high;
    mid -= (std::move(p.high) - std::move(p.low)) * (std::move(q.high) - std::move(q.low));
    return std::move(low) +
           (std::move(mid) << sz * 32) +
           (std::move(high) << sz * 32 * 2);
}

namespace
{
    struct Toom3Mat
    {
        BigInt zero, one, negone, negtwo, inf;
        Toom3Mat(const BigInt &big, const size_t sz)
        {
            BigInt b0, b1, b2;
            size_t i = 0;
            size_t bigSz = big.data.size();
            b0.data.reserve(std::min(bigSz, sz));
            for (; i < bigSz && i < sz; ++i)
            {
                b0.data.push_back(big.data[i]);
            }
            b1.data.reserve(std::min(bigSz - i, sz * 2 - i));
            for (; i < bigSz && i < sz * 2; ++i)
            {
                b1.data.push_back(big.data[i]);
            }
            b2.data.reserve(bigSz - i);
            for (; i < bigSz; ++i)
            {
                b2.data.push_back(big.data[i]);
            }
            auto tmp = b0 + b2;
            zero = b0;
            one = tmp + b1;
            negone = std::move(tmp) - std::move(b1);
            negtwo = ((negone + b2) << 1) - std::move(b0);
            inf = std::move(b2);
        }
    };
}

static BigInt div2(BigInt &&big)
{
    if (big == NegOne())
        big = Zero();
    else
        big >>= 1;
    return std::move(big);
}

BigInt BigInt::toom3(const BigInt &lhs, const BigInt &rhs)
{
    const auto sz = ceilDiv(std::max(lhs.data.size(), rhs.data.size()), 3);
    Toom3Mat p(lhs, sz);
    Toom3Mat q(rhs, sz);
    p.zero *= q.zero;
    p.one *= q.one;
    p.negone *= q.negone;
    p.negtwo *= q.negtwo;
    p.inf *= q.inf;
    auto r0 = p.zero;
    auto r4 = p.inf;
    auto r3 = (std::move(p.negtwo) - p.one) / Three();
    auto r1 = div2(std::move(p.one) - p.negone);
    auto r2 = std::move(p.negone) - std::move(p.zero);
    r3 = div2(r2 - r3) + (std::move(p.inf) << 1);
    r2 += r1 - r4;
    r1 -= r3;
    return std::move(r0) +
           (std::move(r1) << sz * 32) +
           (std::move(r2) << sz * 32 * 2) +
           (std::move(r3) << sz * 32 * 3) +
           (std::move(r4) << sz * 32 * 4);
}
