import argparse
import contextlib
import io
import random
import subprocess
import sys
import time


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
        (y, sz) = get_num(low, high)
        x = get_num(sz + 1, high + 1)[0]
    else:
        x = get_num(low, high)[0]
        y = get_num(low, high)[0]
    if x_neg:
        x = -x
    if y_neg:
        y = -y
    res = {"+": add, "-": sub, "*": mul, "/": c_div, "%": c_mod}[op](x, y)
    return f"{x} {op} {y} {res}"


def to_stdout(count, low, high):
    for _ in range(count):
        print(gen_line(low, high))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exec")
    parser.add_argument("count", type=int)
    parser.add_argument("low", type=int)
    parser.add_argument("high", type=int)
    args = parser.parse_args()
    sys.set_int_max_str_digits(10000)
    start = time.perf_counter()
    with io.StringIO() as input:
        with contextlib.redirect_stdout(input):
            to_stdout(args.count, args.low, args.high)
        py_done = time.perf_counter()
        print(f"Python took {py_done - start} seconds")
        completed = subprocess.run([args.exec], input=input.getvalue(), text=True)
        bigint_done = time.perf_counter()
        print(f"BigIntStress took {bigint_done - py_done} seconds")
    sys.exit(completed.returncode)


if __name__ == "__main__":
    main()
