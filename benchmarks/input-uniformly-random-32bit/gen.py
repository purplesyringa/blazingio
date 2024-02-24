import random

print("\n".join(
    str(random.randint(-2 ** 31, 2 ** 31 - 1))
    for _ in range(5000000)
))
