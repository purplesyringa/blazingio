import random
import sys

table = bytes(random.randint(33, 255) for _ in range(256))
sys.stdout.buffer.write(random.randbytes(100000000).translate(table))
