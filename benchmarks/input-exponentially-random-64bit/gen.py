import random

print("\n".join(
    random.choice(("", "-")) + str(int(2 ** random.uniform(0, 63)))
    for _ in range(10000000)
))
