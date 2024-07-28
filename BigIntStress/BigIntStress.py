import argparse
import contextlib
import random


def add(x, y):
    return x + y


def sub(x, y):
    return x - y


def mul(x, y):
    return x * y


def c_div(x, y):
    neg = (x < 0) != (y < 0)
    if x < 0:
        x = -x
    if y < 0:
        y = -y
    ans = x // y
    return -ans if neg else ans


def c_mod(x, y):
    neg = x < 0
    if x < 0:
        x = -x
    if y < 0:
        y = -y
    ans = x % y
    return -ans if neg else ans


def get_num(low, high):
    x = random.randint(low, high)
    return (random.randint(2 ** ((x - 1) * 32), 2 ** (x * 32)), x)


def gen_line(low, high):
    op = random.choice(("+", "-", "*", "/", "%"))
    x_neg = random.choice((True, False))
    y_neg = random.choice((True, False))
    if op in {"/", "%"}:
        (x, sz) = get_num(low, high)
        y = get_num(sz + 1, high + 1)[0]
    else:
        x = get_num(low, high)[0]
        y = get_num(low, high)[0]
    if x_neg:
        x = -x
    if y_neg:
        y = -y
    res = {"+": add, "-": sub, "*": mul, "/": c_div, "%": c_mod}[op](x, y)
    return f"{x} {op} {y} {res}"


def to_stdout(count):
    for _ in range(count):
        print(gen_line(1, 100))


def to_file(count):
    with open("generated.txt", "w") as file:
        with contextlib.redirect_stdout(file):
            to_stdout(count)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("count", type=int)
    parser.add_argument("low", type=int)
    parser.add_argument("high", type=int)
    args = parser.parse_args()
    print(args)


if __name__ == "__main__":
    main()
