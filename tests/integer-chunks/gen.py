import sys

# Split signs, digits, and delimiters at common pipe and buffer boundaries.
tokens = ["-9223372036854775808", "9223372036854775807", "0"]
cases = [(size, offset, token) for size in (4096, 8192, 65536)
         for offset in range(-21, 2) for token in tokens]
parts = [str(len(cases) + 1) + "\n"]
position = len(parts[0])
for size, offset, token in cases:
    padding = (offset - position) % size
    parts.extend([" " * padding, token, "\n"])
    position += padding + len(token) + 1
# A token can span multiple chunks; finish without a trailing newline.
parts.append("0" * 131072 + "123")
sys.stdout.write("".join(parts))
