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
#include <stdexcept>
#include <sstream>
#include <unordered_set>
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
            },
        },
        {
            "~",
            [&]()
            {
                auto &vals = regs[0];
                BigInt rhs;
                if (top1(vals, rhs))
                    vals.push_back(~std::move(rhs));
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
    std::unordered_map<std::string, std::function<void(std::size_t)>> regOps{
        {
            "s",
            [&](const std::size_t i)
            {
                auto &vals = regs[i];
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
            [&](const std::size_t i)
            {
                auto &vals = regs[i];
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
            [&](const std::size_t i)
            {
                auto &vals = regs[i];
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
            [&](const std::size_t i)
            {
                auto &vals = regs[i];
                if (vals.size() >= 1)
                {
                    vals.pop_back();
                }
            },
        },
        {
            "c",
            [&](const std::size_t i)
            {
                auto &vals = regs[i];
                if (vals.size() >= 1)
                {
                    vals.push_back(vals.back());
                }
            },
        },
    };
    std::unordered_set<std::string> outOps{"l", "t", "h", "quit"};
    auto outList = [&](const std::size_t i)
    {
        auto &vals = regs[i];
        for (const auto &val : vals)
        {
            std::cout << out(val) << '\n';
        }
    };
    auto outTop = [&](const std::size_t i)
    {
        auto &vals = regs[i];
        for (auto j = vals.size() > 2 ? vals.size() - 2 : 0; j < vals.size(); ++j)
        {
            std::cout << out(vals[j]) << '\n';
        }
    };
    auto outHelp = [&]()
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
    };
    std::unordered_map<std::string, std::function<void(std::size_t)>> memOps{
        {
            "st",
            [&](const std::size_t i)
            {
                auto &main = regs[0];
                auto &vals = regs[i];
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
            [&](const std::size_t i)
            {
                auto &main = regs[0];
                auto &vals = regs[i];
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
    auto prompt = [](std::stringstream &ss)
    {
        std::cout << "> ";
        std::string line;
        auto res = static_cast<bool>(std::getline(std::cin, line));
        ss.clear();
        ss.str(line);
        return res;
    };
    std::stringstream ss;
    while (prompt(ss))
    {
        std::string input;
        std::size_t lastIdx = 0;
        std::string outType = "t";
        while (ss >> input)
        {
            lastIdx = 0;
            outType = "t";
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
                    lastIdx = idx.value_or(0);
                    regIter->second(idx.value_or(0));
                    continue;
                }
                if (auto memIter = memOps.find(op); memIter != memOps.end())
                {
                    memIter->second(idx.value_or(1));
                    continue;
                }
                if (auto outIter = outOps.find(op); outIter != outOps.end())
                {
                    lastIdx = idx.value_or(0);
                    outType = *outIter;
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
            std::cout << "unknown op " << input << '\n';
        }
        switch (outType.size() ? outType[0] : '\0')
        {
        case 'l':
            outList(lastIdx);
            break;
        case 't':
            outTop(lastIdx);
            break;
        case 'h':
            outHelp();
            break;
        case 'q':
            goto end;
            break;
        }
    }
    std::cout << '\n';
end:
    std::cout << "goodbye\n";
}
