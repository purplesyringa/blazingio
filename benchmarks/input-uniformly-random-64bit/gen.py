import random

print("\n".join(
    str(random.randint(-2 ** 63, 2 ** 63 - 1))
    for _ in range(5000000)
))
