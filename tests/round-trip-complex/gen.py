import random


print(10000)

total_re = 0
total_im = 0

for _ in range(10000):
    re = random.randint(0, 1000)
    im = random.randint(0, 1000)
    total_re += re
    total_im += im
    print(f"({re},{im})")

print(f"({total_re},{total_im})")
