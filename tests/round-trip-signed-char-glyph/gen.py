import random
import sys

for _ in range(2):
	# Exclude whitespace
	s = bytes(random.randint(33, 255) for _ in range(10000))
	sys.stdout.buffer.write(s)
	print(sum(s), flush=True)
