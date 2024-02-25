import random

print("\n".join(
    str(random.randint(1, 2 ** 31 - 1) * random.choice((-1, 1)))
    for _ in range(5000000)
))
