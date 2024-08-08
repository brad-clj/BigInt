#include <cctype>
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>
#include "BigInt.h"

static bool prompt(std::stringstream &ss)
{
    std::cout << "> ";
    std::string line;
    auto res = static_cast<bool>(std::getline(std::cin, line));
    ss.clear();
    ss.str(line);
    return res;
}

static bool top2(std::deque<BigInt> &vals, BigInt &lhs, BigInt &rhs)
{
    if (vals.size() < 2)
        return false;
    rhs = std::move(vals.back());
    vals.pop_back();
    lhs = std::move(vals.back());
    vals.pop_back();
    return true;
}

static bool top1(std::deque<BigInt> &vals, BigInt &top)
{
    if (vals.size() < 1)
        return false;
    top = std::move(vals.back());
    vals.pop_back();
    return true;
}

static bool fromString(std::string_view str, BigInt &out)
{
    try
    {
        out = BigInt::fromString(str);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

static bool fromHex(std::string_view str, BigInt &out)
{
    try
    {
        out = BigInt::fromHex(str);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

struct Calc
{
    using RegsType = std::vector<std::deque<BigInt>>;
    RegsType regs = RegsType(10);
    bool hex = false;

    std::string out(const BigInt &val)
    {
        return hex ? val.toHex() : val.toString();
    }

    void run()
    {
        std::stringstream ss;
        while (prompt(ss))
        {
            std::string input;
            std::size_t lastIdx = 0;
            std::string outOp = "t";
            while (ss >> input)
            {
                lastIdx = 0;
                outOp = "t";
                if (input.size() && std::isalpha(input[0]))
                {
                    std::string_view sv = input;
                    std::optional<std::size_t> idx;
                    if (std::isdigit(sv.back()))
                    {
                        idx = static_cast<std::size_t>(sv.back() - '0');
                        sv.remove_suffix(1);
                    }
                    std::string op(sv);
                    if (auto mainIter = mainOps.find(op); mainIter != mainOps.end())
                    {
                        mainIter->second(*this);
                        continue;
                    }
                    if (auto regIter = regOps.find(op); regIter != regOps.end())
                    {
                        lastIdx = idx.value_or(0);
                        regIter->second(*this, idx.value_or(0));
                        continue;
                    }
                    if (auto memIter = memOps.find(op); memIter != memOps.end())
                    {
                        memIter->second(*this, idx.value_or(1));
                        continue;
                    }
                    if (auto outIter = outOps.find(op); outIter != outOps.end())
                    {
                        lastIdx = idx.value_or(0);
                        outOp = std::move(op);
                        continue;
                    }
                }
                if (auto mathIter = mathOps.find(input); mathIter != mathOps.end())
                {
                    mathIter->second(*this);
                    continue;
                }
                BigInt val;
                if (fromString(input, val))
                {
                    regs[0].push_back(val);
                    continue;
                }
                if (fromHex(input, val))
                {
                    regs[0].push_back(val);
                    continue;
                }
                std::cout << "unknown op " << input << '\n';
            }
            if (auto outIter = outOps.find(outOp); outIter != outOps.end())
            {
                if (outIter->second(*this, lastIdx))
                    return;
            }
        }
        std::cout << '\n';
    }

    static const std::unordered_map<std::string, void (*)(Calc &)> mathOps;
    static const std::unordered_map<std::string, void (*)(Calc &)> mainOps;
    static const std::unordered_map<std::string, void (*)(Calc &, std::size_t)> regOps;
    static const std::unordered_map<std::string, void (*)(Calc &, std::size_t)> memOps;
    static const std::unordered_map<std::string, bool (*)(Calc &, std::size_t)> outOps;
};

static void math(Calc &calc, const std::function<BigInt(BigInt &&lhs, BigInt &&rhs)> &fn)
{
    BigInt lhs, rhs;
    auto &vals = calc.regs[0];
    if (top2(vals, lhs, rhs))
    {
        BigInt res;
        try
        {
            res = fn(std::move(lhs), std::move(rhs));
        }
        catch (const std::invalid_argument &e)
        {
            std::cout << "exception: " << e.what() << '\n';
            vals.push_back(std::move(lhs));
            vals.push_back(std::move(rhs));
            return;
        }
        vals.push_back(std::move(res));
    }
}

static void mathAdd(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) + std::move(rhs); });
}

static void mathSub(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) - std::move(rhs); });
}

static void mathMul(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) * std::move(rhs); });
}

static void mathPow(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return BigInt::pow(std::move(lhs), std::move(rhs).toInteger()); });
}

static void mathDiv(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) / std::move(rhs); });
}

static void mathMod(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) % std::move(rhs); });
}

static void mathDivmod(Calc &calc)
{
    BigInt lhs, rhs;
    auto &vals = calc.regs[0];
    if (top2(vals, lhs, rhs))
    {
        DivModRes res;
        try
        {
            res = BigInt::divmod(std::move(lhs), std::move(rhs));
        }
        catch (const std::invalid_argument &e)
        {
            std::cout << "exception: " << e.what() << '\n';
            vals.push_back(std::move(lhs));
            vals.push_back(std::move(rhs));
            return;
        }
        vals.push_back(std::move(res.q));
        vals.push_back(std::move(res.r));
    }
}

static void mathNot(Calc &calc)
{
    auto &vals = calc.regs[0];
    BigInt rhs;
    if (top1(vals, rhs))
        vals.push_back(~std::move(rhs));
}

static void mathAnd(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) & std::move(rhs); });
}

static void mathOr(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) | std::move(rhs); });
}

static void mathXor(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) ^ std::move(rhs); });
}

static void mathShiftL(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) << std::move(rhs).toInteger(); });
}

static void mathShiftR(Calc &calc)
{
    math(calc,
         [](BigInt &&lhs, BigInt &&rhs)
         { return std::move(lhs) >> std::move(rhs).toInteger(); });
}

const std::unordered_map<std::string, void (*)(Calc &)> Calc::mathOps{
    {"+", mathAdd},
    {"-", mathSub},
    {"*", mathMul},
    {"**", mathPow},
    {"/", mathDiv},
    {"%", mathMod},
    {"/%", mathDivmod},
    {"~", mathNot},
    {"&", mathAnd},
    {"|", mathOr},
    {"^", mathXor},
    {"<<", mathShiftL},
    {">>", mathShiftR},
};

static void mainHex(Calc &calc)
{
    calc.hex = true;
}

static void mainDec(Calc &calc)
{
    calc.hex = false;
}

static void mainReset(Calc &calc)
{
    calc.regs = Calc::RegsType(10);
}

const std::unordered_map<std::string, void (*)(Calc &)> Calc::mainOps{
    {"hex", mainHex},
    {"dec", mainDec},
    {"reset", mainReset},
};

static void regSwap(Calc &calc, std::size_t i)
{
    auto &vals = calc.regs[i];
    BigInt lhs, rhs;
    if (top2(vals, lhs, rhs))
    {
        vals.push_back(std::move(rhs));
        vals.push_back(std::move(lhs));
    }
}

static void regRotateUp(Calc &calc, std::size_t i)
{
    auto &vals = calc.regs[i];
    if (vals.size() >= 2)
    {
        auto &val = vals.front();
        vals.push_back(std::move(val));
        vals.pop_front();
    }
}

static void regRotateDown(Calc &calc, std::size_t i)
{
    auto &vals = calc.regs[i];
    if (vals.size() >= 2)
    {
        auto &val = vals.back();
        vals.push_front(std::move(val));
        vals.pop_back();
    }
}

static void regPop(Calc &calc, std::size_t i)
{
    auto &vals = calc.regs[i];
    if (vals.size() >= 1)
    {
        vals.pop_back();
    }
}

static void regCopy(Calc &calc, std::size_t i)
{
    auto &vals = calc.regs[i];
    if (vals.size() >= 1)
    {
        vals.push_back(vals.back());
    }
}

const std::unordered_map<std::string, void (*)(Calc &, std::size_t)> Calc::regOps{
    {"s", regSwap},
    {"u", regRotateUp},
    {"d", regRotateDown},
    {"p", regPop},
    {"c", regCopy},
};

static void memStore(Calc &calc, std::size_t i)
{
    auto &main = calc.regs[0];
    auto &vals = calc.regs[i];
    if (main.size() >= 1)
    {
        auto val = std::move(main.back());
        main.pop_back();
        vals.push_back(std::move(val));
    }
}

static void memLoad(Calc &calc, std::size_t i)
{
    auto &main = calc.regs[0];
    auto &vals = calc.regs[i];
    if (vals.size() >= 1)
    {
        auto val = std::move(vals.back());
        vals.pop_back();
        main.push_back(std::move(val));
    }
}

const std::unordered_map<std::string, void (*)(Calc &, std::size_t)> Calc::memOps{
    {"st", memStore},
    {"ld", memLoad},
};

static bool outList(Calc &calc, const std::size_t i)
{
    auto &vals = calc.regs[i];
    for (const auto &val : vals)
    {
        std::cout << calc.out(val) << '\n';
    }
    return false;
}

static bool outTop(Calc &calc, const std::size_t i)
{
    auto &vals = calc.regs[i];
    for (auto j = vals.size() > 2 ? vals.size() - 2 : 0; j < vals.size(); ++j)
    {
        std::cout << calc.out(vals[j]) << '\n';
    }
    return false;
}

static bool outHelp(Calc &, std::size_t)
{
    std::cout << "There are 10 stacks. 0 is the primary stack and math ops are\n"
              << "only available to stack 0. l, t, and stack ops default to 0,\n"
              << "and memory ops default to 1. But those ops can be applied to\n"
              << "a specific stack by adding a digit suffix to the op (e.g. s1\n"
              << "to swap on stack 1).\n"
              << '\n'
              << "math ops:\n"
              << "    +, -, *, **, /, %, /%, ~, &, |, ^, <<, >>\n"
              << "stack ops:\n"
              << "    s (swap), u (rotate up), d (rotate down), p (pop), c (copy)\n"
              << "memory ops:\n"
              << "    st (store), ld (load)\n"
              << "output ops:\n"
              << "    l (list), t (top), dec, hex\n"
              << "reset (to clear everything), quit (to quit)\n";
    return false;
}

static bool outQuit(Calc &, std::size_t) { return true; }

const std::unordered_map<std::string, bool (*)(Calc &, std::size_t)> Calc::outOps{
    {"l", outList},
    {"t", outTop},
    {"h", outHelp},
    {"quit", outQuit},
};

int main()
{
    std::cout << "welcome, enter h for help\n";
    Calc calc;
    calc.run();
    std::cout << "goodbye\n";
}
