import random


def round_trip(n, l, r):
    print(n)
    for _ in range(n):
        print(random.randint(l, r - 1), end=" ")
    print()


round_trip(10000, -2 ** 7, 2 ** 7)
round_trip(10000, 0, 2 ** 8)
