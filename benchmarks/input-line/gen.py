import random
import sys

sys.stdout.buffer.write(b"\n".join(
    random.randbytes(random.randint(0, 2000))
    for _ in range(100000)
))
