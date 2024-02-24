import random

print("\n".join(
    random.choice(("", "-")) + str(int(2 ** random.uniform(0, 15)))
    for _ in range(5000000)
))
