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
    chunks.reserve(count);
    isNeg = num < 0;
    while (num != 0 && num != -1)
    {
        chunks.push_back(static_cast<uint32_t>(num));
        num >>= 32;
    }
}

BigInt::BigInt(std::string_view str)
{
    constexpr auto exceptionMsg = "BigInt string_view ctor has invalid argument";
    auto strIsNeg = str.size() && str[0] == '-';
    if (strIsNeg)
        str = str.substr(1);
    if (str.size() == 0)
        throw std::invalid_argument(exceptionMsg);
    auto sub = str.substr(0, str.size() % 18);
    int64_t tmp;
    if (sub.size())
    {
        auto res = std::from_chars(sub.data(), sub.data() + sub.size(), tmp);
        if (res.ec != std::errc{} || res.ptr != sub.data() + sub.size())
            throw std::invalid_argument(exceptionMsg);
        *this += tmp;
        str = str.substr(str.size() % 18);
    }
    while (str.size())
    {
        *this *= OneExa();
        sub = str.substr(0, 18);
        auto res = std::from_chars(sub.data(), sub.data() + sub.size(), tmp);
        if (res.ec != std::errc{} || res.ptr != sub.data() + sub.size())
            throw std::invalid_argument(exceptionMsg);
        *this += tmp;
        str = str.substr(18);
    }
    if (strIsNeg)
        negate();
}

BigInt &BigInt::operator+=(const BigInt &rhs)
{
    if (this == &rhs)
        return *this += BigInt(rhs);
    size_t i = 0;
    chunks.reserve(std::max(chunks.size(), rhs.chunks.size()) + 1);
    auto lim = chunks.size();
    for (; i < rhs.chunks.size(); ++i)
    {
        addChunk(i, rhs.chunks[i]);
    }
    if (rhs.isNeg)
    {
        while (i < lim)
        {
            addChunk(i++, static_cast<uint32_t>(-1));
        }
        if (i < chunks.size())
            chunks.pop_back();
        else
        {
            if (isNeg)
                addChunk(i, static_cast<uint32_t>(-1));
            isNeg = true;
        }
    }
    normalize();
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
    size_t i = 0;
    chunks.reserve(std::max(chunks.size(), rhs.chunks.size()) + 1);
    auto lim = chunks.size();
    for (; i < rhs.chunks.size(); ++i)
    {
        subChunk(i, rhs.chunks[i]);
    }
    if (rhs.isNeg)
    {
        while (i < lim)
        {
            subChunk(i++, static_cast<uint32_t>(-1));
        }
        if (i < chunks.size())
            chunks.pop_back();
        else
        {
            if (!isNeg)
                subChunk(i, static_cast<uint32_t>(-1));
            isNeg = false;
        }
    }
    normalize();
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
    return *this = *this / rhs;
}

BigInt &BigInt::operator%=(const BigInt &rhs)
{
    return *this = *this % rhs;
}

static void bitwise(BigInt &lhs, const BigInt &rhs, std::function<void(uint32_t &, uint32_t)> fn)
{
    auto lim = std::max(lhs.chunks.size(), rhs.chunks.size());
    for (size_t i = 0; i < lim; ++i)
    {
        if (i == lhs.chunks.size())
            lhs.chunks.push_back(lhs.isNeg ? static_cast<uint32_t>(-1) : 0);
        auto &a = lhs.chunks[i];
        auto b = i < rhs.chunks.size() ? rhs.chunks[i]
                 : rhs.isNeg           ? static_cast<uint32_t>(-1)
                                       : 0;
        fn(a, b);
    }
    uint32_t x = lhs.isNeg ? static_cast<uint32_t>(-1) : 0;
    fn(x, rhs.isNeg ? static_cast<uint32_t>(-1) : 0);
    lhs.isNeg = static_cast<bool>(x);
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

template <typename N, typename D>
auto ceilDiv(const N n, const D d)
{
    return n / d + (n % d ? 1 : 0);
}

BigInt &BigInt::operator<<=(const size_t n)
{
    if (n == 0)
        return *this;
    const auto oldSz = chunks.size();
    const auto off = n / 32;
    const auto s = n % 32;
    chunks.resize(chunks.size() + ceilDiv(n, 32));
    for (auto i = chunks.size(); i--;)
    {
        auto x = i < off           ? 0
                 : i - off < oldSz ? chunks[i - off] << s
                 : isNeg           ? static_cast<uint32_t>(-1) << s
                                   : 0;
        if (s)
            x |= i < off + 1 ? 0 : chunks[i - off - 1] >> (32 - s);
        chunks[i] = x;
    }
    normalize();
    return *this;
}

BigInt &BigInt::operator>>=(const size_t n)
{
    if (n == 0)
        return *this;
    if (n >= chunks.size() * 32)
    {
        chunks.clear();
        return *this;
    }
    const auto off = n / 32;
    const auto s = n % 32;
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        auto x = i + off < chunks.size() ? chunks[i + off] >> s
                 : isNeg                 ? static_cast<uint32_t>(-1) >> s
                                         : 0;
        if (s)
            x |= i + off + 1 < chunks.size() ? chunks[i + off + 1] << (32 - s)
                 : isNeg                     ? static_cast<uint32_t>(-1) << (32 - s)
                                             : 0;
        chunks[i] = x;
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

static constexpr size_t Toom2Thresh = 300;
static constexpr size_t Toom3Thresh = 800;

BigInt BigInt::operator*(const BigInt &rhs) const
{
    auto &lhs = *this;
    if (!lhs.isNeg && !rhs.isNeg)
    {
        const auto score = lhs.chunks.size() * rhs.chunks.size();
        if (score > Toom3Thresh)
            return toom3(lhs, rhs);
        if (score > Toom2Thresh)
            return toom2(lhs, rhs);
        BigInt res;
        res.chunks.reserve(lhs.chunks.size() + rhs.chunks.size());
        for (size_t i = 0; i < lhs.chunks.size(); ++i)
        {
            for (size_t j = 0; j < rhs.chunks.size(); ++j)
            {
                auto prod = static_cast<uint64_t>(lhs.chunks[i]) * rhs.chunks[j];
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
        auto res = lhs.isNeg && rhs.isNeg ? -lhs * -rhs
                   : lhs.isNeg            ? -lhs * rhs
                                          : lhs * -rhs;
        if (lhs.isNeg != rhs.isNeg)
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
    return isNeg || chunks.size();
}

bool BigInt::operator==(const BigInt &rhs) const
{
    return this == &rhs ||
           (isNeg == rhs.isNeg && chunks == rhs.chunks);
}

static std::strong_ordering cmp(const BigInt &diff)
{
    return !diff.isNeg && diff.chunks.size() == 0 ? std::strong_ordering::equal
           : diff.isNeg                           ? std::strong_ordering::less
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

std::string BigInt::toString() const &
{
    return BigInt(*this).toString();
}

std::string BigInt::toString() &&
{
    if (*this == Zero())
        return "0";
    std::string res = isNeg ? "-" : "";
    if (isNeg)
        negate();
    std::vector<std::string> digits;
    while (*this)
    {
        auto [q, r] = divmod(*this, OneExa());
        uint64_t val = r.chunks.size() > 1 ? r.chunks[1] : 0;
        val <<= 32;
        val += r.chunks.size() > 0 ? r.chunks[0] : 0;
        digits.push_back(std::to_string(val));
        *this = std::move(q);
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

std::string BigInt::toHex() const &
{
    return BigInt(*this).toHex();
}

std::string BigInt::toHex() &&
{
    if (*this == Zero())
        return "0x0";
    std::string res = isNeg ? "-0x" : "0x";
    if (isNeg)
        negate();
    std::vector<std::string> hexChunks(chunks.size(), std::string(8, '0'));
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        auto &chunk = chunks[i];
        auto &hexChunk = hexChunks[i];
        for (size_t j = hexChunk.size(); j-- && chunk;)
        {
            auto off = static_cast<char>(chunk % 16);
            chunk /= 16;
            hexChunk[j] = off < 10 ? '0' + off
                                   : 'a' + off - 10;
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

void BigInt::normalize()
{
    while (chunks.size() && chunks.back() == (isNeg ? static_cast<uint32_t>(-1) : 0))
    {
        chunks.pop_back();
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
    for (auto &chunk : chunks)
    {
        chunk = ~chunk;
    }
    isNeg = !isNeg;
}

void BigInt::addChunk(size_t i, const uint32_t val)
{
    while (chunks.size() <= i)
    {
        chunks.push_back(isNeg ? static_cast<uint32_t>(-1) : 0);
    }
    auto hasCarry = (chunks[i++] += val) < val;
    while (hasCarry)
    {
        if (i == chunks.size())
        {
            if (isNeg)
            {
                isNeg = false;
                return;
            }
            chunks.push_back(0);
        }
        hasCarry = ++chunks[i++] == 0;
    }
}

void BigInt::subChunk(size_t i, const uint32_t val)
{
    while (chunks.size() <= i)
    {
        chunks.push_back(isNeg ? static_cast<uint32_t>(-1) : 0);
    }
    auto prev = chunks[i];
    auto hasBorrow = (chunks[i++] -= val) > prev;
    while (hasBorrow)
    {
        if (i == chunks.size())
        {
            if (!isNeg)
            {
                isNeg = true;
                return;
            }
            chunks.push_back(static_cast<uint32_t>(-1));
        }
        hasBorrow = --chunks[i++] == static_cast<uint32_t>(-1);
    }
}

BigInt BigInt::fromHex(std::string_view str)
{
    constexpr auto exceptionMsg = "BigInt fromHex has invalid argument";
    BigInt big;
    auto strIsNeg = str.substr(0, 3) == "-0x"  ? true
                    : str.substr(0, 2) == "0x" ? false
                                               : throw std::invalid_argument(exceptionMsg);
    str.remove_prefix(strIsNeg ? 3 : 2);
    if (str.size() == 0)
        throw std::invalid_argument(exceptionMsg);
    big.chunks.reserve(ceilDiv(str.size(), 8));
    while (str.size())
    {
        uint32_t tmp;
        auto sub = str.substr(str.size() > 8 ? str.size() - 8 : 0);
        str.remove_suffix(sub.size());
        auto res = std::from_chars(sub.data(), sub.data() + sub.size(), tmp, 16);
        if (res.ec != std::errc{} || res.ptr != sub.data() + sub.size())
            throw std::invalid_argument(exceptionMsg);
        big.chunks.push_back(tmp);
    }
    if (strIsNeg)
        big.negate();
    big.normalize();
    return big;
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
        for (size_t j = 0; j < d.chunks.size(); ++j)
        {
            uint64_t z = d.chunks[j];
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
    for (size_t j = 0; j < d.chunks.size(); ++j)
    {
        r.addChunk(i + j, d.chunks[j]);
    }
    r.normalize();
}

DivModRes BigInt::divmod(const BigInt &lhs, const BigInt &rhs)
{
    DivModRes res{{}, lhs};
    auto d = rhs;
    if (lhs.isNeg)
        res.r.negate();
    if (rhs.isNeg)
        d.negate();
    const int s = 32 - bitCount(d.chunks.back());
    res.r <<= s;
    d <<= s;
    if (res.r.chunks.size() + 1 > d.chunks.size())
        res.q.chunks.reserve(res.r.chunks.size() + 1 - d.chunks.size());
    const auto x = d.chunks.back();
    for (size_t i = res.r.chunks.size(); i-- >= d.chunks.size();)
    {
        auto y = static_cast<uint64_t>(res.r.chunks[i]);
        y |= i + 1 < res.r.chunks.size() ? static_cast<uint64_t>(res.r.chunks[i + 1]) << 32 : 0;
        auto z = y / x;
        size_t j = i - d.chunks.size() + 1;
        divmodMulSub(res.r, z, d, j);
        while (res.r.isNeg)
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
    if (lhs.isNeg != rhs.isNeg)
        res.q.negate();
    if (lhs.isNeg)
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
            size_t bigSz = big.chunks.size();
            low.chunks.reserve(std::min(bigSz, sz));
            for (; i < bigSz && i < sz; ++i)
            {
                low.chunks.push_back(big.chunks[i]);
            }
            high.chunks.reserve(bigSz - i);
            for (; i < bigSz; ++i)
            {
                high.chunks.push_back(big.chunks[i]);
            }
        }
    };
}

BigInt BigInt::toom2(const BigInt &lhs, const BigInt &rhs)
{
    const auto sz = ceilDiv(std::max(lhs.chunks.size(), rhs.chunks.size()), 2);
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
            size_t bigSz = big.chunks.size();
            b0.chunks.reserve(std::min(bigSz, sz));
            for (; i < bigSz && i < sz; ++i)
            {
                b0.chunks.push_back(big.chunks[i]);
            }
            b1.chunks.reserve(std::min(bigSz - i, sz * 2 - i));
            for (; i < bigSz && i < sz * 2; ++i)
            {
                b1.chunks.push_back(big.chunks[i]);
            }
            b2.chunks.reserve(bigSz - i);
            for (; i < bigSz; ++i)
            {
                b2.chunks.push_back(big.chunks[i]);
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

static BigInt &&div2(BigInt &&big)
{
    if (big == NegOne())
        big = Zero();
    else
        big >>= 1;
    return std::move(big);
}

BigInt BigInt::toom3(const BigInt &lhs, const BigInt &rhs)
{
    const auto sz = ceilDiv(std::max(lhs.chunks.size(), rhs.chunks.size()), 3);
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
