#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include "BigInt.h"

int main()
{
    using OpFn = std::function<bool(const BigInt &x,
                                    const BigInt &y,
                                    const BigInt &res)>;
    std::unordered_map<std::string, OpFn> ops{
        {
            "+",
            [](const BigInt &x,
               const BigInt &y,
               const BigInt &res)
            {
                return x + y == res;
            },
        },
        {
            "-",
            [](const BigInt &x,
               const BigInt &y,
               const BigInt &res)
            {
                return x - y == res;
            },
        },
        {
            "*",
            [](const BigInt &x,
               const BigInt &y,
               const BigInt &res)
            {
                return x * y == res;
            },
        },
        {
            "/",
            [](const BigInt &x,
               const BigInt &y,
               const BigInt &res)
            {
                return x / y == res;
            },
        },
        {
            "%",
            [](const BigInt &x,
               const BigInt &y,
               const BigInt &res)
            {
                return x % y == res;
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
