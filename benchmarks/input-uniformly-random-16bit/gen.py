import random

print("\n".join(
    str(random.randint(-2 ** 15, 2 ** 15 - 1))
    for _ in range(5000000)
))
