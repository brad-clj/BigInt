#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include "BigInt.h"

int main()
{
    using OpFn = std::function<bool(std::string_view x,
                                    std::string_view y,
                                    std::string_view res)>;
    std::unordered_map<std::string, OpFn> ops{
        {
            "+",
            [](std::string_view x,
               std::string_view y,
               std::string_view res)
            {
                return BigInt(x) + BigInt(y) == BigInt(res);
            },
        },
        {
            "-",
            [](std::string_view x,
               std::string_view y,
               std::string_view res)
            {
                return BigInt(x) - BigInt(y) == BigInt(res);
            },
        },
        {
            "*",
            [](std::string_view x,
               std::string_view y,
               std::string_view res)
            {
                return BigInt(x) * BigInt(y) == BigInt(res);
            },
        },
        {
            "/",
            [](std::string_view x,
               std::string_view y,
               std::string_view res)
            {
                return BigInt(x) / BigInt(y) == BigInt(res);
            },
        },
        {
            "%",
            [](std::string_view x,
               std::string_view y,
               std::string_view res)
            {
                return BigInt(x) % BigInt(y) == BigInt(res);
            },
        },
    };
    bool fail = false;
    std::string x, op, y, res;
    while (std::cin >> x >> op >> y >> res)
    {
        if (!ops[op](x, y, res))
        {
            fail = true;
            std::cerr << x << ' '
                      << op << ' '
                      << y << " != "
                      << res << '\n';
        }
    }
    if (fail)
        return 1;
}
