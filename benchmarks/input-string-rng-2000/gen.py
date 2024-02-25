import random
import sys


table = bytes(random.randint(33, 255) for _ in range(256))
sys.stdout.buffer.write(b" ".join(
    random.randbytes(random.randint(1, 2000)).translate(table)
    for _ in range(2000000)
))
