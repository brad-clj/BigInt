#include "BigInt.h"
#include <algorithm>
#include <charconv>
#include <functional>
#include <memory>

const BigInt &BigInt::OneExa()
{
    static std::unique_ptr<const BigInt> OneExaPtr = nullptr;
    if (OneExaPtr == nullptr)
        OneExaPtr.reset(new BigInt(1'000'000'000'000'000'000));
    return *OneExaPtr;
}

BigInt::BigInt() {}

BigInt::BigInt(int64_t num)
{
    negative = num < 0;
    while (num != 0 && num != -1)
    {
        data.push_back(static_cast<uint32_t>(num));
        num >>= 32;
    }
}

BigInt::BigInt(std::string_view str)
{
    auto neg = str[0] == '-';
    if (neg)
        str = str.substr(1);
    auto sub = str.substr(0, str.size() % 18);
    int64_t tmp;
    if (sub.size())
    {
        std::from_chars(sub.data(), sub.data() + sub.size(), tmp);
        *this += tmp;
        str = str.substr(str.size() % 18);
    }
    while (str.size())
    {
        *this *= OneExa();
        sub = str.substr(0, 18);
        std::from_chars(sub.data(), sub.data() + sub.size(), tmp);
        *this += tmp;
        str = str.substr(18);
    }
    if (neg)
        negate();
}

BigInt &BigInt::operator+=(const BigInt &rhs)
{
    size_t i = 0;
    auto lim = data.size() + 1;
    for (; i < rhs.data.size(); ++i)
    {
        addChunk(i, rhs.data[i]);
    }
    if (rhs.negative)
    {
        while (i < lim ||
               (data.back() != 1 &&
                data.back() != static_cast<uint32_t>(-1)))
        {
            addChunk(i++, static_cast<uint32_t>(-1));
        }
        negative = data.back() == static_cast<uint32_t>(-1);
        data.pop_back();
    }
    normalize();
    return *this;
}

BigInt &BigInt::operator-=(const BigInt &rhs)
{
    size_t i = 0;
    auto lim = data.size() + 1;
    for (; i < rhs.data.size(); ++i)
    {
        subChunk(i, rhs.data[i]);
    }
    if (rhs.negative)
    {
        while (i < lim ||
               (data.back() != 0 &&
                data.back() != static_cast<uint32_t>(-2)))
        {
            subChunk(i++, static_cast<uint32_t>(-1));
        }
        negative = data.back() == static_cast<uint32_t>(-2);
        data.pop_back();
    }
    normalize();
    return *this;
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
        auto b = i < rhs.data.size()
                     ? rhs.data[i]
                 : rhs.negative
                     ? static_cast<uint32_t>(-1)
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
    bitwise(*this, rhs,
            [](uint32_t &a, uint32_t b)
            { a &= b; });
    return *this;
}

BigInt &BigInt::operator|=(const BigInt &rhs)
{
    bitwise(*this, rhs,
            [](uint32_t &a, uint32_t b)
            { a |= b; });
    return *this;
}

BigInt &BigInt::operator^=(const BigInt &rhs)
{
    bitwise(*this, rhs,
            [](uint32_t &a, uint32_t b)
            { a ^= b; });
    return *this;
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
        auto x = i < off
                     ? 0
                 : i - off < oldSz
                     ? data[i - off] << s
                 : negative
                     ? static_cast<uint32_t>(-1) << s
                     : 0;
        if (s)
            x |= i < off + 1
                     ? 0
                     : data[i - off - 1] >> (32 - s);
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
        auto x = i + off < data.size()
                     ? data[i + off] >> s
                 : negative
                     ? static_cast<uint32_t>(-1) >> s
                     : 0;
        if (s)
            x |= i + off + 1 < data.size()
                     ? data[i + off + 1] << (32 - s)
                 : negative
                     ? static_cast<uint32_t>(-1) << (32 - s)
                     : 0;
        data[i] = x;
    }
    normalize();
    return *this;
}

BigInt &BigInt::operator++()
{
    return *this += 1;
}

BigInt &BigInt::operator--()
{
    return *this -= 1;
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

BigInt BigInt::operator-() const
{
    auto res = *this;
    res.negate();
    return res;
}

BigInt BigInt::operator+(const BigInt &rhs) const
{
    auto res = *this;
    return res += rhs;
}

BigInt BigInt::operator-(const BigInt &rhs) const
{
    auto res = *this;
    return res -= rhs;
}

BigInt BigInt::operator*(const BigInt &rhs) const
{
    auto &lhs = *this;
    if (!lhs.negative && !rhs.negative)
    {
        BigInt res;
        for (size_t i = 0; i < lhs.data.size(); ++i)
        {
            for (size_t j = 0; j < rhs.data.size(); ++j)
            {
                auto prod = static_cast<uint64_t>(lhs.data[i]) * rhs.data[j];
                res.addChunk(i + j, static_cast<uint32_t>(prod));
                res.addChunk(i + j + 1, static_cast<uint32_t>(prod >> 32));
            }
        }
        res.normalize();
        return res;
    }
    else
    {
        auto res = lhs.negative && rhs.negative
                       ? -lhs * -rhs
                   : lhs.negative
                       ? -lhs * rhs
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

BigInt BigInt::operator~() const
{
    auto res = *this;
    res.invert();
    return res;
}

BigInt BigInt::operator&(const BigInt &rhs) const
{
    auto res = *this;
    return res &= rhs;
}

BigInt BigInt::operator|(const BigInt &rhs) const
{
    auto res = *this;
    return res |= rhs;
}

BigInt BigInt::operator^(const BigInt &rhs) const
{
    auto res = *this;
    return res ^= rhs;
}

BigInt BigInt::operator<<(const size_t n) const
{
    auto res = *this;
    return res <<= n;
}

BigInt BigInt::operator>>(const size_t n) const
{
    auto res = *this;
    return res >>= n;
}

BigInt::operator bool() const
{
    return negative || data.size();
}

bool BigInt::operator==(const BigInt &rhs) const
{
    return negative == rhs.negative && data == rhs.data;
}

bool BigInt::operator!=(const BigInt &rhs) const
{
    return negative != rhs.negative || data != rhs.data;
}

bool BigInt::operator<(const BigInt &rhs) const
{
    if (negative != rhs.negative)
        return negative;
    auto diff = *this - rhs;
    return diff.negative;
}

bool BigInt::operator>(const BigInt &rhs) const
{
    return rhs < *this;
}

bool BigInt::operator<=(const BigInt &rhs) const
{
    return !(*this > rhs);
}

bool BigInt::operator>=(const BigInt &rhs) const
{
    return !(*this < rhs);
}

std::string BigInt::toString() const
{
    if (*this == 0)
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
        num = q;
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
    if (val == 0)
        return;
    while (data.size() <= i)
    {
        data.push_back(negative ? static_cast<uint32_t>(-1) : 0);
    }
    auto carry = (data[i++] += val) < val;
    while (carry)
    {
        if (data.size() == i)
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
    if (val == 0)
        return;
    while (data.size() <= i)
    {
        data.push_back(negative ? static_cast<uint32_t>(-1) : 0);
    }
    auto prev = data[i];
    auto borrow = (data[i++] -= val) > prev;
    while (borrow)
    {
        if (data.size() == i)
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
            r.subChunk(i + j, static_cast<uint32_t>(z));
            r.subChunk(i + j + 1, static_cast<uint32_t>(z >> 32));
        }
    }
}

static void divmodAddBack(BigInt &r, const BigInt &d, const size_t i)
{
    for (size_t j = 0; j < d.data.size(); ++j)
    {
        r.addChunk(i + j, d.data[j]);
    }
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
    const auto x = d.data.back();
    for (size_t i = res.r.data.size(); i-- >= d.data.size();)
    {
        auto y = static_cast<uint64_t>(res.r.data[i]);
        y |= i + 1 < res.r.data.size()
                 ? static_cast<uint64_t>(res.r.data[i + 1]) << 32
                 : 0;
        auto z = y / x;
        size_t j = i - d.data.size() + 1;
        divmodMulSub(res.r, z, d, j);
        while (res.r.negative)
        {
            --z;
            divmodAddBack(res.r, d, j);
        }
        res.q.addChunk(j, static_cast<uint32_t>(z));
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
