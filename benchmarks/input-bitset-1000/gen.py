import random

print("\n".join(
    "".join(random.choice("01") for _ in range(1000))
    for _ in range(100000)
))
