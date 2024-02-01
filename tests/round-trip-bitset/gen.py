import random


def round_trip(n):
    s = "".join(random.choice("01") for _ in range(n))
    print(s)

    hash = 0
    for c in s[::-1]:
        hash *= 57
        hash += int(c)
        hash %= 2 ** 64

    print(hash)


round_trip(0)
round_trip(1)
round_trip(15)
round_trip(32)
round_trip(64)
round_trip(128)
round_trip(256)
round_trip(300)
round_trip(5000)
