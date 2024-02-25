import random
import sys

sys.stdout.buffer.write(b" ".join(
    bytes(random.randint(33, 255) for _ in range(random.randint(1, 100)))
    for _ in range(5000000)
))
