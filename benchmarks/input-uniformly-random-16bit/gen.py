import random

print("\n".join(
    str(random.randint(1, 2 ** 15 - 1) * random.choice((-1, 1)))
    for _ in range(20000000)
))
