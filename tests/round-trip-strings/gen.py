import random
import sys


for _ in range(1000):
    length = random.randint(1, 1000)
    s = bytes(random.choice(b"abcdef\x80\xff") for _ in range(length))
    sys.stdout.buffer.write(s + b"\n")
