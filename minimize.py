import re
import subprocess
import sys

from common import CONFIG_OPTS, ARCHITECTURES, OSES


if len(sys.argv) >= 2 and sys.argv[1] == "--override":
    lines = sys.argv[2:]
else:
    lines = []
    for line in open("config"):
        line = line.strip()
        if line and not line.startswith("#"):
            lines.append(line)

opts = []
target_oses = []
target_architectures = []
target_bases = []
for line in lines:
    if line.startswith("os="):
        for os in line.partition("=")[2].split(","):
            if os not in OSES:
                print(f"Invalid OS {os}")
                raise SystemExit(1)
            target_oses.append(os)
    elif line.startswith("architecture="):
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
    else:
        if line not in CONFIG_OPTS:
            print(f"Invalid key-value combination {line}")
            raise SystemExit(1)
        opt = CONFIG_OPTS[line]
        if opt:
            opts.append(opt)


blazingio = open("blazingio.hpp").read()

# Figure out per-architecture/OS cases
needed_factor_macros = set()

def does_selector_match(selector):
    selector_os, selector_arch = selector.split("-")
    if selector_os != "*" and selector_os not in target_oses:
        return False
    if selector_arch != "*":
        if selector_arch not in (target_architectures if "+" in selector_arch else target_bases):
            return False
    return True

def generate_multicase_code(cases):
    filtered_cases = []
    for arg, code in cases:
        selectors, *flags = arg.split()
        for selector in selectors.split(","):
            if not does_selector_match(selector):
                continue
            selector_os, selector_arch = selector.split("-")
            selector_base = selector_arch.split("+")[0]
            filtered_cases.append((selector_os, selector_base, code, flags))

    if not filtered_cases:
        raise ValueError(f"No matching cases in {cases}")

    def factor_out(cases, both_cond, cond, macro):
        true_cases = []
        false_cases = []
        for case in cases:
            if both_cond(case):
                raise StopIteration
            (true_cases if cond(case) else false_cases).append(case)
        if not true_cases or not false_cases:
            raise StopIteration
        needed_factor_macros.add(macro)
        return f"{macro}({codegen(true_cases)}, {codegen(false_cases)})"

    factors = [
        lambda cases: factor_out(
            cases,
            lambda case: case[1] == "*",
            lambda case: case[1] == "x86_64",
            "IF_X86_64"
        ),
        lambda cases: factor_out(
            cases,
            lambda case: case[0] == "*",
            lambda case: case[0] == "windows",
            "IF_WINDOWS"
        ),
        lambda cases: factor_out(
            cases,
            lambda case: case[0] == "*",
            lambda case: case[0] == "macos",
            "IF_MACOS"
        ),
    ]

    def codegen(cases):
        assert cases
        if len(cases) == 1:
            _, _, code, flags = cases[0]
            if "wrap" in flags:
                needed_factor_macros.add("UNWRAP")
                code = f"UNWRAP({code})"
            return code
        for factor in factors:
            try:
                return factor(cases)
            except StopIteration:
                pass
        raise StopIteration

    try:
        return codegen(filtered_cases)
    except StopIteration:
        raise ValueError(f"Cannot factor cases {filtered_cases}") from None


def handler(match):
    if match[0].startswith("@match\n"):
        text = match[0].removeprefix("@match\n").removesuffix("@end\n")
        first, *rest = re.split(r"@case (.*)\n", text)
        assert first == ""
        return generate_multicase_code(list(zip(rest[::2], rest[1::2]))) + "\n"
    elif match[0].startswith("@ondemand "):
        selectors = match[0].split()[1]
        code = match[0].removeprefix(f"@ondemand {selectors}\n").removesuffix("@end\n")
        if any(does_selector_match(selector) for selector in selectors.split(",")):
            return code
        else:
            return ""
    elif match[0].startswith("@define "):
        name = match[0].split()[1]
        text = match[0].removeprefix(f"@define {name}\n").removesuffix("@end\n")
        cases = []
        for line in text.splitlines():
            assert line.startswith("@case ")
            _, selectors, *value = line.split(" ", 2)
            cases.append((selectors, " ".join(value)))
        value = generate_multicase_code(cases)
        # Just a heuristic
        prefix = "!" if len(value) < 10 else "#"
        return f"{prefix}define {name} {value}\n"
    elif match[0].startswith("@include\n"):
        text = match[0].removeprefix("@include\n").removesuffix("@end\n")
        # We can't conditionally include a header or not, so include <ios> in the negative case. It
        # is the shortest name, and it is already included transitively by <iostream> so compilation
        # time shouldn't be affected.
        cases = []
        for line in text.splitlines():
            assert line.startswith("@case ")
            _, selectors, include = line.split(" ")
            cases.append((selectors, "<ios>" if include == "none" else include))
        code = generate_multicase_code(cases)
        if code == "<ios>":
            return ""
        else:
            return f"#include {code}\n"
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

# Replace "return *this;"
blazingio = "#define $r return*this;\n" + re.sub(r"return\s*\*this;", "$r", blazingio)

# Replace various common keywords
blazingio = "#define $O operator\n" + blazingio.replace("operator", "$O")
blazingio = "#define $R return\n" + blazingio.replace("return", "$R")
blazingio = "#define $C constexpr\n" + blazingio.replace("constexpr", "$C")
blazingio = "#define $c class\n" + blazingio.replace("class", "$c").replace("typename", "$c")
blazingio = "#define $T template<\n" + re.sub(r"template\s*<", "$T ", blazingio)

# Add multiarch/multiOS support
if "IF_X86_64" in needed_factor_macros:
    blazingio = "#ifdef __x86_64__\n#define IF_X86_64(yes, no) yes\n#else\n#define IF_X86_64(yes, no) no\n#endif\n" + blazingio
if "IF_WINDOWS" in needed_factor_macros:
    blazingio = "#ifdef _WIN32\n#define IF_WINDOWS(yes, no) yes\n#else\n#define IF_WINDOWS(yes, no) no\n#endif\n" + blazingio
if "IF_MACOS" in needed_factor_macros:
    blazingio = "#ifdef __APPLE__\n#define IF_MACOS(yes, no) yes\n#else\n#define IF_MACOS(yes, no) no\n#endif\n" + blazingio
if "UNWRAP" in needed_factor_macros:
    blazingio = "#define UNWRAP(...) __VA_ARGS__\n" + blazingio

# Strip out comments
blazingio = re.sub(r"//.*", "", blazingio)


def repl(s):
    # Replace libc constants
    consts = {
        "PROT_READ": 1,
        "PROT_WRITE": 2,
        "MAP_PRIVATE": 2,
        "MAP_FIXED": 0x10,
        "MAP_ANONYMOUS": 0x20,
        "MAP_NORESERVE": 0x4000,
        "MADV_POPULATE_READ": 22,
        "STDIN_FILENO": 0,
        "STDOUT_FILENO": 1,
        "SEEK_END": 2,
        "SPLICE_F_GIFT": 8,
        "SYS_read": generate_multicase_code([
            ("linux-x86_64", "0"),
            ("linux-aarch64", "63"),
            ("macos-*", "3"),  # this is not documented anywhere, but it's been 3 for dozens of years
        ])
    }
    const = "(" + "|".join(consts) + ")"
    s = re.sub(
        const + r"(\|" + const + ")*", lambda match: str(eval(match[0], consts)), s
    )

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
        ("IF_X86_64", "$S"),
        ("IF_WINDOWS", "$w"),
        ("IF_MACOS", "$m"),
        ("UNWRAP", "$u"),

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
        ("yes", "A"),

        ("NonAliasingChar", "B"),
        ("exponent", "B"),
        ("imag_part", "B"),
        ("interactive", "B"),
        ("iov", "B"),
        ("MinDigits", "B"),
        ("whole", "B"),
        ("stream", "B"),
        ("no", "B"),
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
        ("table", "D"),

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

    return s

blazingio = "".join(
    repl(part) if part[0] != '"' else part for part in re.split(r"(\".*?\")", blazingio)
)


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
