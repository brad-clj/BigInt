#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include "BigInt.h"

int main()
{
    using OpFn = std::function<bool(BigInt && x,
                                    BigInt && y,
                                    const BigInt &res)>;
    std::unordered_map<std::string, OpFn> ops{
        {
            "+",
            [](BigInt &&x,
               BigInt &&y,
               const BigInt &res)
            {
                return std::move(x) + std::move(y) == res;
            },
        },
        {
            "-",
            [](BigInt &&x,
               BigInt &&y,
               const BigInt &res)
            {
                return std::move(x) - std::move(y) == res;
            },
        },
        {
            "*",
            [](BigInt &&x,
               BigInt &&y,
               const BigInt &res)
            {
                return std::move(x) * std::move(y) == res;
            },
        },
        {
            "/",
            [](BigInt &&x,
               BigInt &&y,
               const BigInt &res)
            {
                return std::move(x) / std::move(y) == res;
            },
        },
        {
            "%",
            [](BigInt &&x,
               BigInt &&y,
               const BigInt &res)
            {
                return std::move(x) % std::move(y) == res;
            },
        },
    };
    bool fail = false;
    std::string x, op, y, res;
    while (std::cin >> x >> op >> y >> res)
    {
        if (!ops[op](BigInt::fromHex(x),
                     BigInt::fromHex(y),
                     BigInt::fromHex(res)))
        {
            fail = true;
            std::cerr << x << ' '
                      << op << ' '
                      << y << ' '
                      << res << '\n';
        }
    }
    if (fail)
        return 1;
}
