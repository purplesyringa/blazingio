import re
import subprocess
import sys

from common import CONFIG_OPTS, ARCHITECTURES


if len(sys.argv) >= 2 and sys.argv[1] == "--override":
    lines = sys.argv[2:]
else:
    lines = []
    for line in open("config"):
        line = line.strip()
        if line and not line.startswith("#"):
            lines.append(line)

opts = []
target_architectures = []
target_bases = []
for line in lines:
    if line.startswith("architecture="):
        for architecture in line.partition("=")[2].split(","):
            base, *extensions = architecture.split("+")
            if base not in ARCHITECTURES:
                print(f"Invalid architecture {base}")
                raise SystemExit(1)
            if len(extensions) > 1:
                print("Extensions cannot be combined for one architecture")
                raise SystemExit(1)
            if extensions and extensions[0] not in ARCHITECTURES[base]:
                print(f"Invalid extension {extensions[0]} for architecture {base}")
                raise SystemExit(1)
            if extensions:
                target_architectures.append(architecture)
            else:
                target_architectures.append(architecture + "+none")
            target_bases.append(base)
        continue
    if line not in CONFIG_OPTS:
        print(f"Invalid key-value combination {line}")
        raise SystemExit(1)
    opt = CONFIG_OPTS[line]
    if opt:
        opts.append(opt)


blazingio = open("blazingio.hpp").read()

# Figure out per-architecture cases
def handler(match):
    if match[0].startswith("@match\n"):
        text = match[0].removeprefix("@match\n").removesuffix("@end\n")
        first, *rest = re.split(r"@case (.*)\n", text)
        assert first == ""
        x86_64_code = None
        x86_64_flags = None
        aarch64_code = None
        aarch64_flags = None
        for case_arg, code in zip(rest[::2], rest[1::2]):
            selectors, *flags = case_arg.split()
            for selector in selectors.split(","):
                if "+" in selector:
                    possible = selector in target_architectures
                else:
                    possible = selector in target_bases
                if not possible:
                    continue
                base = selector.split("+")[0]
                if base == "x86_64":
                    x86_64_code = code
                    x86_64_flags = flags
                elif base == "aarch64":
                    aarch64_code = code
                    aarch64_flags = flags
                else:
                    assert False

        if x86_64_code is not None and aarch64_code is not None and x86_64_code != aarch64_code:
            if "wrap" in x86_64_flags:
                x86_64_code = f"UNWRAP({x86_64_code})"
            if "wrap" in aarch64_flags:
                aarch64_code = f"UNWRAP({aarch64_code})"
            return f"SELECT_ARCH({x86_64_code}, {aarch64_code})"
        elif x86_64_code is not None:
            return x86_64_code
        elif aarch64_code is not None:
            return aarch64_code
        else:
            return ""
    elif match[0].startswith("@ondemand "):
        selectors = match[0].split()[1]
        code = match[0].removeprefix(f"@ondemand {selectors}\n").removesuffix("@end\n")
        possible = False
        for selector in selectors.split(","):
            if "+" in selector:
                possible = possible or selector in target_architectures
            else:
                possible = possible or selector in target_bases
        if possible:
            return code
        else:
            return ""
    elif match[0].startswith("@define "):
        name = match[0].split()[1]
        text = match[0].removeprefix(f"@define {name}\n").removesuffix("@end\n")
        x86_64_value = None
        aarch64_value = None
        for line in text.splitlines():
            assert line.startswith("@case ")
            _, selectors, *value = line.split(" ", 2)
            for selector in selectors.split(","):
                if "+" in selector:
                    possible = selector in target_architectures
                else:
                    possible = selector in target_bases
                if not possible:
                    continue
                base = selector.split("+")[0]
                if base == "x86_64":
                    x86_64_value = " ".join(value)
                elif base == "aarch64":
                    aarch64_value = " ".join(value)
                else:
                    assert False
        if x86_64_value is not None and aarch64_value is not None and x86_64_value != aarch64_value:
            value = f"SELECT_ARCH({x86_64_value}, {aarch64_value})"
        elif x86_64_value is not None:
            value = x86_64_value
        elif aarch64_value is not None:
            value = aarch64_value
        else:
            return ""
        # Just a heuristic
        prefix = "!" if len(value) < 10 else "#"
        return f"{prefix}define {name} {value}\n"
    elif match[0].startswith("@include\n"):
        text = match[0].removeprefix("@include\n").removesuffix("@end\n")
        x86_64_include = None
        aarch64_include = None
        for line in text.splitlines():
            assert line.startswith("@case ")
            _, selectors, include = line.split(" ")
            for selector in selectors.split(","):
                if "+" in selector:
                    possible = selector in target_architectures
                else:
                    possible = selector in target_bases
                if not possible:
                    continue
                base = selector.split("+")[0]
                if base == "x86_64":
                    x86_64_include = include
                elif base == "aarch64":
                    aarch64_include = include
                else:
                    assert False
        if x86_64_include is not None and aarch64_include is not None and x86_64_include != aarch64_include:
            # We can't conditionally include a header or not, so include <ios> in the negative case. It
            # is the shortest name, and it is already included transitively by <iostream> so compilation
            # time shouldn't be affected.
            if x86_64_include == "none":
                x86_64_include = "<ios>"
            if aarch64_include == "none":
                aarch64_include = "<ios>"
            return f"#include SELECT_ARCH({x86_64_include}, {aarch64_include})\n"
        elif x86_64_include is not None and x86_64_include != "none":
            return f"#include {x86_64_include}\n"
        elif aarch64_include is not None and aarch64_include != "none":
            return f"#include {aarch64_include}\n"
        else:
            return ""
    else:
        assert False

blazingio = re.sub(r"(@match|@ondemand .*|@define .*|@include)\n[\s\S]*?@end\n", handler, blazingio)

# Preprocess
blazingio = re.sub(r"^#", "cpp#", blazingio, flags=re.M)
blazingio = re.sub(r"^!", "#", blazingio, flags=re.M)
proc = subprocess.run(
    ["cpp", "-P"] + [f"-D{opt}" for opt in opts],
    input=blazingio.encode(),
    capture_output=True,
    check=True,
)
blazingio = proc.stdout.decode()
blazingio = re.sub(r"^cpp#", "#", blazingio, flags=re.M)

# Remove unnecessary parentheses
proc = subprocess.run(
    ["clang-format-18", "--style=file:minimize.clang-format"],
    input=blazingio.encode(),
    capture_output=True,
    check=True,
)
blazingio = proc.stdout.decode()

# Replace "return *this;"
blazingio = "#define $r return*this;\n" + re.sub(r"return\s*\*this;", "$r", blazingio)

# Replace various common keywords
blazingio = "#define $O operator\n" + blazingio.replace("operator", "$O")
blazingio = "#define $R return\n" + blazingio.replace("return", "$R")
blazingio = "#define $C constexpr\n" + blazingio.replace("constexpr", "$C")
blazingio = "#define $c class\n" + blazingio.replace("class", "$c").replace("typename", "$c")
blazingio = "#define $T template<\n" + re.sub(r"template\s*<", "$T ", blazingio)

if len(target_architectures) > 1:
    # Add multiarch support
    blazingio = "#ifdef __x86_64__\n#define SELECT_ARCH(x86_64, aarch64) x86_64\n#else\n#define SELECT_ARCH(x86_64, aarch64) aarch64\n#endif\n#define UNWRAP(...) __VA_ARGS__\n" + blazingio

# Strip out comments
blazingio = re.sub(r"//.*", "", blazingio)


# Remove unnecessary whitespace
def whitespace(s):
    if s.startswith("#"):
        s, *rest = s.split(None, 1)
        if rest:
            s += " " + whitespace(rest[0])
        else:
            s += "\n"
        return s
    for _ in range(3):
        s = re.sub(r"([^a-zA-Z0-9_$])\s+(\S)", r"\1\2", s)
        s = re.sub(r"(\S)\s+([^a-zA-Z0-9_$])", r"\1\2", s)
    s = s.replace("\n", " ")
    s = s.replace("''", "' '")
    s = s.strip()
    if s:
        s += "\n"
    return s


blazingio = "".join(whitespace(part) for part in re.split(r"(#.*)", blazingio))

# Remove whitespace after "#include"
blazingio = re.sub(r"#include\s+<", "#include<", blazingio)


# Replace character literals with their values
blazingio = re.sub(
    r"(?<!<<)(?<!print\()('\\?.')", lambda match: str(ord(eval(match[1]))), blazingio
)


def repl(s):
    # Replace identifiers
    for old, new in [
        ("blazingio_istream", "$i"),
        ("blazingio_ostream", "$o"),
        ("blazingio_ignoreostream", "$e"),
        ("blazingio_cin", "i$"),
        ("blazingio_cout", "o$"),
        ("blazingio_cerr", "e$"),
        ("blazingio_init", "t$"),
        ("blazingio_freopen", "f$"),
        ("ensure", "E$"),
        ("blazingio", "$f"),
        ("SIMD", "$s"),
        ("SIMD_SIZE", "$z"),
        ("SIMD_TYPE", "$t"),
        ("INLINE", "$I"),
        ("FETCH", "$F"),
        ("SELECT_ARCH", "$S"),
        ("UNWRAP", "$u"),

        ("UninitChar", "A"),
        ("Inner", "A"),
        ("n_read", "A"),
        ("negative", "A"),
        ("vec", "A"),
        ("line", "A"),
        ("real_part", "A"),
        ("file", "A"),
        ("decimal_lut", "A"),
        ("n_written", "A"),
        ("computed", "A"),
        ("abs", "A"),
        ("write12", "A"),
        ("func", "A"),
        ("x86_64", "A"),
        ("table", "A"),

        ("NonAliasingChar", "B"),
        ("exponent", "B"),
        ("imag_part", "B"),
        ("interactive", "B"),
        ("iov", "B"),
        ("MinDigits", "B"),
        ("whole", "B"),
        ("stream", "B"),
        ("aarch64", "B"),
        ("masked", "B"),

        ("ONE_BYTES", "C"),
        ("file_size", "C"),
        ("rsi", "C"),
        ("has_dot", "C"),
        ("trace", "C"),
        ("space", "C"),
        ("vec1", "C"),
        ("MaxDigits", "C"),
        ("zipped", "C"),

        ("line_t", "D"),
        ("base", "D"),
        ("new_exponent", "D"),
        ("exps", "D"),
        ("vec2", "D"),
        ("Factor", "D"),

        ("buffer", "E"),
        ("mask", "E"),
        ("do_flush", "E"),
        ("interval", "E"),

        ("Interactive", "F"),
        ("print", "F"),
        ("low_digits", "F"),

        ("istream_impl", "G"),
        ("start", "G"),
        ("write_int_split", "G"),

        ("end", "H"),
        ("coeff", "H"),

        ("ptr", "I"),

        ("blazingio_istream", "J"),

        ("value", "K"),
        ("init_assume_file", "K"),

        ("init_assume_interactive", "L"),

        ("fetch", "M"),

        ("collect_digits", "N"),

        ("input", "O"),

        ("input_string_like", "P"),

        ("rshift_impl", "Q"),

        ("BITSET_SHIFT", "R"),

        ("NULL", "0"),
        ("false", "0"),
        ("true", "1"),
        ("MAP_FAILED", "(void*)-1"),
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
        "STDOUT_FILENO": 1,
        "SEEK_END": 2,
        "SPLICE_F_GIFT": 8,
        "SIGBUS": 7,
        "SYS_read": 0,
    }
    const = "(" + "|".join(consts) + ")"
    s = re.sub(
        const + r"(\|" + const + ")*", lambda match: str(eval(match[0], consts)), s
    )

    return s


blazingio = "".join(
    repl(part) if part[0] != '"' else part for part in re.split(r"(\".*?\")", blazingio)
)

# Replace hexadecimal integer literals
blazingio = re.sub(
    r"0x([0-9a-f]+)",
    lambda match: str(int(match[0], 16))
    if len(str(int(match[0], 16))) < 2 + len(match[1].lstrip("0"))
    else "0x" + match[1].lstrip("0"),
    blazingio,
)

# Replace SIMD intrinsics
if "_mm256_" in blazingio:
    blazingio = "#define M$(x,...)_mm256_##x##_epi8(__VA_ARGS__)\n" + re.sub(
        r"_mm256_(\w+)_epi8\(", r"M$(\1,", blazingio
    )
    blazingio = "#define L$(x)_mm256_loadu_si256(x)\n" + blazingio.replace(
        "_mm256_loadu_si256(", "L$("
    )
elif "_mm_" in blazingio:
    blazingio = "#define M$(x,...)_mm_##x##_epi8(__VA_ARGS__)\n" + re.sub(
        r"_mm_(\w+)_epi8\(", r"M$(\1,", blazingio
    )
    blazingio = "#define L$(x)_mm_loadu_si128(x)\n" + blazingio.replace(
        "_mm_loadu_si128(", "L$("
    )

blazingio = blazingio.strip()

# Add comments
blazingio = f"// DO NOT REMOVE THIS MESSAGE. The mess that follows is a compressed build of\n// https://github.com/purplesyringa/blazingio. Refer to the repository for\n// a human-readable version and documentation.\n// Config options: {' '.join(opts) if opts else '(none)'}\n// Targets: {', '.join(target_architectures)}\n{blazingio}\n// End of blazingio\n"

open("blazingio.min.hpp", "w").write(blazingio)

print("Wrote", len(blazingio), "bytes to blazingio.min.hpp")
