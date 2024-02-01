import random


def round_trip(n, l, r):
    print(n)
    total = 0
    for _ in range(n):
        x = random.randint(l, r - 1)
        total += x
        print(x, end=" ")
    print()
    print(total % (2 ** 64))


round_trip(10000, -2 ** 15, 2 ** 15)
round_trip(10000, 0, 2 ** 16)
round_trip(10000, -2 ** 31, 2 ** 31)
round_trip(10000, 0, 2 ** 32)
round_trip(10000, -2 ** 63, 2 ** 63)
round_trip(10000, 0, 2 ** 64)
