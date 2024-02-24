import random

print("\n".join(
    random.choice(("", "-")) + str(10 ** random.uniform(-20, 20))
    for _ in range(5000000)
))
