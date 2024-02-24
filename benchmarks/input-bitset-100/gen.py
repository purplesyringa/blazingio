import random

print("\n".join(
    "".join(random.choice("01") for _ in range(100))
    for _ in range(500000)
))
