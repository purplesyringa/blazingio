import re
import subprocess


blazingio = open("blazingio.hpp").read()

# Preprocess
blazingio = re.sub(r"^#", "cpp#", blazingio, flags=re.M)
blazingio = re.sub(r"^cpp#\t", "#", blazingio, flags=re.M)
proc = subprocess.run(["cpp", "-P"], input=blazingio.encode(), capture_output=True, check=True)
blazingio = proc.stdout.decode()
blazingio = re.sub(r"^cpp#", "#", blazingio, flags=re.M)

# Remove unnecessary parentheses
proc = subprocess.run(["clang-format-18", "--style=file:minimize.clang-format"], input=blazingio.encode(), capture_output=True, check=True)
blazingio = proc.stdout.decode()

# Replace "return *this;"
blazingio = "#define $r return*this;}\n" + re.sub(r"return\s*\*this;\s*}", "$r", blazingio)

# Strip out comments
blazingio = re.sub(r"//.*", "", blazingio)

# Remove unnecessary whitespace
def whitespace(s):
	if s.startswith("#"):
		s, *rest = s.split(None, 1)
		if rest:
			s += " " + whitespace(rest[0])
		return s
	for _ in range(2):
		s = re.sub(r"(\W)\s+(\S)", r"\1\2", s)
		s = re.sub(r"(\S)\s+(\W)", r"\1\2", s)
	s = s.replace("\n", " ")
	s = s.replace("''", "' '")
	s = s.strip()
	if s:
		s += "\n"
	return s

blazingio = "".join(whitespace(part) for part in re.split(r"(#.*)", blazingio))

def repl(s):
	# Replace identifiers
	for (old, new) in [
		("blazingio_istream", "$i"),
		("blazingio_ostream", "$o"),
		("blazingio_ignoreostream", "$e"),
		("blazingio_cin", "i$"),
		("blazingio_cout", "o$"),
		("blazingio_cerr", "e$"),
		("blazingio_init", "t$"),
		("ensure", "E$"),
		("blazingio", "$f"),
		("SIMD", "$s"),
		("typename", "class"),
		("UninitChar", "A"),
		("NonAliasingChar", "B"),
		("trace_non_whitespace", "C"),
		("trace_line", "D"),
		("init", "E"),
		("on_eof", "F"),
		("skip_whitespace", "G"),
		("collect_digits", "H"),
		("read_arithmetic", "I"),
		("stream", "J"),
		("ptr", "K"),
		("on_sigbus", "L"),
		("file_size", "M"),
		("base", "N"),
		("value", "O"),
		("abs", "P"),
		("coeff", "Q"),
		("interval", "R"),
		("MinDigits", "S"),
		("MaxDigits", "U"),
		("decimal_lut", "V"),
		("write_int_split", "W"),
		("whole", "X"),
		("func", "Y"),
		("act", "Z"),
		("info", "a"),
		("in", "b"),
		("start", "d"),
		("is_ok", "e"),
		("exponent", "f"),
		("low_digits", "g"),
		("Factor", "h"),
		("computed", "j"),
		("n_written", "k"),
		("negative", "l"),
		("exps", "m"),
		("after_dot", "o"),
		("vec1", "q"),
		("vec2", "r"),
		("vec", "s"),
		("mask", "t"),
		("fd_exe", "u"),
		("want_alloc_size", "v"),
		("alloc_size", "w"),
		("n_read", "y"),
		("statbuf", "z"),
		("fd", "q"),
		("Inner", "q"),
		("empty_fd", "U"),
		("NULL", "0"),
		("false", "0"),
		("true", "1"),
		("MAP_FAILED", "(void*)-1")
	]:
		s = re.sub(r"\b" + re.escape(old) + r"\b", new, s)

	# Replace libc constants
	consts = {
		"O_RDONLY": 0,
		"O_RDWR": 2,
		"PROT_READ": 1,
		"PROT_WRITE": 2,
		"MAP_SHARED": 1,
		"MAP_PRIVATE": 2,
		"MAP_FIXED": 0x10,
		"MAP_ANONYMOUS": 0x20,
		"MAP_NORESERVE": 0x4000,
		"MAP_POPULATE": 0x8000,
		"MADV_POPULATE_READ": 22,
		"MADV_POPULATE_WRITE": 23,
		"SA_SIGINFO": 4,
		"STDIN_FILENO": 0,
		"STDOUT_FILENO": 1
	}
	const = "(" + "|".join(consts) + ")"
	s = re.sub(const + r"(\|" + const + ")*", lambda match: str(eval(match[0], consts)), s)

	return s

blazingio = "".join(repl(part) if part[0] != "\"" else part for part in re.split(r"(\".*?\")", blazingio))

# Replace character literals with their values
blazingio = re.sub(r"(?<!<<)('\\?.')", lambda match: str(ord(eval(match[1]))), blazingio)

# Replace hexadecimal integer literals
blazingio = re.sub(r"0x([0-9a-f]+)", lambda match: str(int(match[0], 16)) if len(str(int(match[0], 16))) < 2 + len(match[1].lstrip("0")) else "0x" + match[1].lstrip("0"), blazingio)

# Replace SIMD intrinsics
if "_mm256_" in blazingio:
	blazingio = "#define M$(x,...)_mm256_##x##_epi8(__VA_ARGS__)\n" + re.sub(r"_mm256_(\w+)_epi8\(", r"M$(\1,", blazingio)
	blazingio = "#define L$(x)_mm256_loadu_si128((__m256i*)x)\n" + blazingio.replace("_mm256_loadu_si256((__m256i*)", "L$(")
elif "_mm_" in blazingio:
	blazingio = "#define M$(x,...)_mm_##x##_epi8(__VA_ARGS__)\n" + re.sub(r"_mm_(\w+)_epi8\(", r"M$(\1,", blazingio)
	blazingio = "#define L$(x)_mm_loadu_si128((__m128i*)x)\n" + blazingio.replace("_mm_loadu_si128((__m128i*)", "L$(")


open("blazingio.min.hpp", "w").write(blazingio)

print(blazingio, len(blazingio))
