import random

print("\n".join(
    str(random.uniform(-1e20, 1e20))
    for _ in range(5000000)
))
