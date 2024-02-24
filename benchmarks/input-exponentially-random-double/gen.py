import random

print("\n".join(
    random.choice(("", "-")) + str(10 ** random.uniform(-300, 300))
    for _ in range(5000000)
))
