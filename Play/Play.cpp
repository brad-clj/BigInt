#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <utility>
#include <string_view>
#include <deque>
#include <vector>
#include <cctype>
#include <optional>
#include "BigInt.h"

int main()
{
    std::cout << "welcome, enter h for help\n";
    std::vector<std::deque<BigInt>> regs(10);
    auto top2 = [&](std::deque<BigInt> &vals, BigInt &lhs, BigInt &rhs)
    {
        if (vals.size() < 2)
            return false;
        rhs = std::move(vals.back());
        vals.pop_back();
        lhs = std::move(vals.back());
        vals.pop_back();
        return true;
    };
    auto top1 = [&](std::deque<BigInt> &vals, BigInt &top)
    {
        if (vals.size() < 1)
            return false;
        top = std::move(vals.back());
        vals.pop_back();
        return true;
    };
    bool hex = false;
    auto out = [&](const BigInt &val)
    { return hex ? val.toHex() : val.toString(); };
    auto math = [&](const std::function<BigInt(BigInt &&lhs, BigInt &&rhs)> &fn)
    {
        BigInt lhs, rhs;
        auto &vals = regs[0];
        if (top2(vals, lhs, rhs))
        {
            auto res = fn(std::move(lhs), std::move(rhs));
            std::cout << out(res) << '\n';
            vals.push_back(std::move(res));
        }
    };
    std::unordered_map<std::string, std::function<void()>> mathOps{
        {
            "+",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) + std::move(rhs); });
            },
        },
        {
            "-",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) - std::move(rhs); });
            },
        },
        {
            "*",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) * std::move(rhs); });
            },
        },
        {
            "**",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return BigInt::pow(std::move(lhs), std::move(rhs).toInteger()); });
            },
        },
        {
            "/",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) / std::move(rhs); });
            },
        },
        {
            "/%",
            [&]()
            {
                auto &vals = regs[0];
                BigInt lhs, rhs;
                if (top2(vals, lhs, rhs))
                {
                    auto [q, r] = BigInt::divmod(std::move(lhs), std::move(rhs));
                    std::cout << out(q) << '\n';
                    vals.push_back(std::move(q));
                    std::cout << out(r) << '\n';
                    vals.push_back(std::move(r));
                }
            },
        },
        {
            "~",
            [&]()
            {
                auto &vals = regs[0];
                BigInt rhs;
                if (top1(vals, rhs))
                {
                    auto res = ~std::move(rhs);
                    std::cout << out(res) << '\n';
                    vals.push_back(std::move(res));
                }
            },
        },
        {
            "&",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) & std::move(rhs); });
            },
        },
        {
            "|",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) | std::move(rhs); });
            },
        },
        {
            "^",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) ^ std::move(rhs); });
            },
        },
        {
            "<<",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) << std::move(rhs).toInteger(); });
            },
        },
        {
            ">>",
            [&]()
            {
                math([](BigInt &&lhs, BigInt &&rhs)
                     { return std::move(lhs) >> std::move(rhs).toInteger(); });
            },
        },
    };
    std::unordered_map<std::string, std::function<void()>> mainOps{
        {
            "h",
            [&]()
            {
                std::cout << "math ops: + - * ** / % /% ~ & | ^ << >>\n"
                          << "stack ops: s (swap), u (rotate up), d (rotate down), p (pop), c (copy)\n"
                          << "memory ops: st (store), ld (load)\n"
                          << "output ops: l (list), t (top), dec, hex\n"
                          << "reset (to clear everything)\n";
            },
        },
        {
            "hex",
            [&]()
            { hex = true; },
        },
        {
            "dec",
            [&]()
            { hex = false; },
        },
        {
            "reset",
            [&]()
            { regs = std::vector<std::deque<BigInt>>(10); },
        },
    };
    std::unordered_map<std::string, std::function<void(const std::optional<std::size_t> &)>> regOps{
        {
            "s",
            [&](const std::optional<std::size_t> &i)
            {
                auto &vals = regs[i.value_or(0)];
                BigInt lhs, rhs;
                if (top2(vals, lhs, rhs))
                {
                    vals.push_back(std::move(rhs));
                    vals.push_back(std::move(lhs));
                }
            },
        },
        {
            "u",
            [&](const std::optional<std::size_t> &i)
            {
                auto &vals = regs[i.value_or(0)];
                if (vals.size() >= 2)
                {
                    auto &val = vals.front();
                    vals.push_back(std::move(val));
                    vals.pop_front();
                }
            },
        },
        {
            "d",
            [&](const std::optional<std::size_t> &i)
            {
                auto &vals = regs[i.value_or(0)];
                if (vals.size() >= 2)
                {
                    auto &val = vals.back();
                    vals.push_front(std::move(val));
                    vals.pop_back();
                }
            },
        },
        {
            "p",
            [&](const std::optional<std::size_t> &i)
            {
                auto &vals = regs[i.value_or(0)];
                if (vals.size() >= 1)
                {
                    vals.pop_back();
                }
            },
        },
        {
            "c",
            [&](const std::optional<std::size_t> &i)
            {
                auto &vals = regs[i.value_or(0)];
                if (vals.size() >= 1)
                {
                    vals.push_back(vals.back());
                }
            },
        },
        {
            "l",
            [&](const std::optional<std::size_t> &i)
            {
                auto &vals = regs[i.value_or(0)];
                for (const auto &val : vals)
                {
                    std::cout << out(val) << '\n';
                }
            },
        },
        {
            "t",
            [&](const std::optional<std::size_t> &i)
            {
                auto &vals = regs[i.value_or(0)];
                for (auto j = vals.size() > 2 ? vals.size() - 2 : 0; j < vals.size(); ++j)
                {
                    std::cout << out(vals[j]) << '\n';
                }
            },
        },
        {
            "st",
            [&](const std::optional<std::size_t> &i)
            {
                auto &main = regs[0];
                auto &vals = regs[i.value_or(1)];
                if (main.size() >= 1)
                {
                    auto val = std::move(main.back());
                    main.pop_back();
                    vals.push_back(std::move(val));
                }
            },
        },
        {
            "ld",
            [&](const std::optional<std::size_t> &i)
            {
                auto &main = regs[0];
                auto &vals = regs[i.value_or(1)];
                if (vals.size() >= 1)
                {
                    auto val = std::move(vals.back());
                    vals.pop_back();
                    main.push_back(std::move(val));
                }
            },
        },
    };
    auto fs = [](std::string_view str, BigInt &out)
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
    };
    auto fh = [](std::string_view str, BigInt &out)
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
    };
    std::string input;
    while (std::cin >> input)
    {
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
                mainIter->second();
                continue;
            }
            if (auto regIter = regOps.find(op); regIter != regOps.end())
            {
                regIter->second(idx);
                continue;
            }
        }
        if (auto mathIter = mathOps.find(input); mathIter != mathOps.end())
        {
            mathIter->second();
            continue;
        }
        BigInt val;
        if (fs(input, val))
        {
            regs[0].push_back(val);
            continue;
        }
        if (fh(input, val))
        {
            regs[0].push_back(val);
            continue;
        }
        std::cout << "invalid op " << input << '\n';
    }
}
