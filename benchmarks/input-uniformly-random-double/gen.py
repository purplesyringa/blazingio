import random

print("\n".join(
    str(random.uniform(-1e300, 1e300))
    for _ in range(5000000)
))
