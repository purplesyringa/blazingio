import re
import subprocess
import os
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
config_str = ""
for line in lines:
    if line.startswith("os="):
        for os_ in line.partition("=")[2].split(","):
            if os_ not in OSES:
                print(f"Invalid OS {os_}")
                raise SystemExit(1)
            target_oses.append(os_)
            config_str += OSES[os_]
    elif line.startswith("architecture="):
        for architecture in line.partition("=")[2].split(","):
            base, *extensions = architecture.split("+")
            if base not in ARCHITECTURES:
                print(f"Invalid architecture {base}")
                raise SystemExit(1)
            arch_extensions, base_str = ARCHITECTURES[base]
            if len(extensions) > 1:
                print("Extensions cannot be combined for one architecture")
                raise SystemExit(1)
            if extensions and extensions[0] not in arch_extensions:
                print(f"Invalid extension {extensions[0]} for architecture {base}")
                raise SystemExit(1)
            config_str += base_str
            if extensions:
                config_str += arch_extensions[extensions[0]]
                target_architectures.append(architecture)
            else:
                target_architectures.append(architecture + "+none")
            target_bases.append(base)
    else:
        if line not in CONFIG_OPTS:
            print(f"Invalid key-value combination {line}")
            raise SystemExit(1)
        opt, opt_str = CONFIG_OPTS[line]
        if opt:
            opts.append(opt)
        config_str += opt_str


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

def does_selector_match_particular(selector, os, base):
    selector_os, selector_arch = selector.split("-")
    return (
        selector_os in ("*", os)
        and selector_os in target_oses
        and (
            selector_arch == "*"
            or (
                selector_arch.split("+")[0] == base
                and selector_arch in (target_architectures if "+" in selector_arch else target_bases)
            )
        )
    )

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
        return None

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
        if all(case[2:] == cases[0][2:] for case in cases):
            _, _, code, flags = cases[0]
            if "wrap" in flags:
                needed_factor_macros.add("UNWRAP")
                code = f"UNWRAP({code})"
            return code
        variants = []
        for factor in factors:
            try:
                variants.append(factor(cases))
            except StopIteration:
                pass
        if not variants:
            raise StopIteration
        return min(variants, key=len)

    try:
        return codegen(filtered_cases)
    except StopIteration:
        raise ValueError(f"Cannot factor cases {filtered_cases}") from None


def handler(match):
    if match[0].startswith("@match\n"):
        text = match[0].removeprefix("@match\n").removesuffix("@end\n")
        first, *rest = re.split(r"@case (.*)\n", text)
        assert first == ""
        code = generate_multicase_code(list(zip(rest[::2], rest[1::2])))
        if code is None:
            return ""
        return code + "\n"
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
        if value is None:
            return ""
        if name.startswith("!") or name.startswith("#"):
            prefix, name = name[0], name[1:]
        else:
            # Heuristic
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
        if code is None or code == "<ios>":
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
    os.environ.get("CPP", "cpp") + " -P" + "".join(f" -D{opt}" for opt in opts),
    input=blazingio.encode(),
    shell=True,
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
    cond = "__x86_64__"
    if "windows" in target_oses:
        cond += " | _M_X64"
    blazingio = f"#if {cond}\n#define IF_X86_64(yes, no) yes\n#else\n#define IF_X86_64(yes, no) no\n#endif\n" + blazingio
if "IF_WINDOWS" in needed_factor_macros:
    blazingio = "#if _WIN32\n#define IF_WINDOWS(yes, no) yes\n#else\n#define IF_WINDOWS(yes, no) no\n#endif\n" + blazingio
if "IF_MACOS" in needed_factor_macros:
    blazingio = "#if __APPLE__\n#define IF_MACOS(yes, no) yes\n#else\n#define IF_MACOS(yes, no) no\n#endif\n" + blazingio
if "UNWRAP" in needed_factor_macros:
    blazingio = "#define UNWRAP(...) __VA_ARGS__\n" + blazingio

# Strip out comments
blazingio = re.sub(r"//.*", "", blazingio)

# Replace character literals with their values
blazingio = re.sub(
    r"(?<!<< )(?<!print\()('\\?.')", lambda match: str(ord(eval(match[1]))), blazingio
)


consts = {
    "PROT_READ": 1,
    "PROT_WRITE": 2,
    "MAP_PRIVATE": 2,
    "MAP_FIXED": 0x10,
    "MAP_ANONYMOUS": {
        "linux-*": 0x20,
        "macos-*": 0x1000,
    },
    "MAP_NORESERVE": {
        "linux-*": 0x4000,
        "macos-*": 0x40,
    },
    "STDIN_FILENO": 0,
    "STDOUT_FILENO": 1,
    "SEEK_SET": 0,
    "SEEK_END": 2,
    "SPLICE_F_GIFT": 8,
    "SYS_read": {
        "linux-x86_64": 0,
        "linux-aarch64": 63,
        # This is not documented anywhere, but it's been this for dozens of years
        "macos-x86_64": (2 << 24) | 3,
        "macos-aarch64": 3,
    },
    "PAGE_NOACCESS": 1,
    "PAGE_READONLY": 2,
    "PAGE_READWRITE": 4,
    "PAGE_GUARD": 0x100,
    "MEM_COMMIT": 0x1000,
    "MEM_RESERVE": 0x2000,
    "MEM_RELEASE": 0x8000,
    "FILE_MAP_READ": 4,
    "STD_INPUT_HANDLE": -10,
    "STD_OUTPUT_HANDLE": -11,
    "STATUS_GUARD_PAGE_VIOLATION": 0x80000001,
    "EXCEPTION_CONTINUE_SEARCH": 0,
    "EXCEPTION_CONTINUE_EXECUTION": -1,
    "GENERIC_WRITE": 0x40000000,
    "FILE_SHARE_READ": 1,
    "FILE_SHARE_WRITE": 2,
    "FILE_SHARE_DELETE": 4,
    "FILE_FLAG_WRITE_THROUGH": 0x80000000,
    "FILE_FLAG_NO_BUFFERING": 0x20000000,
}

def repl(s):
    # Replace libc constants
    def replace_consts(match):
        cases = []
        for os in target_oses:
            for base in target_bases:
                total_value = 0
                for name in match[0].split("|"):
                    values = consts[name.strip()]
                    if isinstance(values, int):
                        value = values
                    else:
                        for selector, value in values.items():
                            if does_selector_match_particular(selector, os, base):
                                break
                        else:
                            total_value = None
                            break
                    if total_value is None:
                        break
                    total_value |= value
                if total_value is not None:
                    cases.append((f"{os}-{base}", str(total_value)))
        return generate_multicase_code(cases) or ""
    const = "(" + "|".join(consts) + ")"
    s = re.sub(const + r"(\s*\|\s*" + const + ")*", replace_consts, s)

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
        ("vectored_exception_handler", "$x"),
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
        ("mmaped_region_size", "A"),
        ("exception_info", "A"),
        ("do_trace", "A"),
        ("mask", "A"),
        ("length", "A"),

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
        ("trigger_address", "B"),

        ("ONE_BYTES", "C"),
        ("file_size", "C"),
        ("rsi", "C"),
        ("syscall_no", "C"),
        ("has_dot", "C"),
        ("trace", "C"),
        ("space", "C"),
        ("vec1", "C"),
        ("MaxDigits", "C"),
        ("zipped", "C"),
        ("exception_record", "C"),
        ("handle", "C"),

        ("line_t", "D"),
        ("base", "D"),
        ("new_exponent", "D"),
        ("exps", "D"),
        ("vec2", "D"),
        ("Factor", "D"),
        ("table", "D"),
        ("arg1", "D"),
        ("stat_buf", "D"),

        ("buffer", "E"),
        ("do_flush", "E"),
        ("interval", "E"),
        ("stdout_handle", "E"),
        ("tmp_n_read", "E"),

        ("Interactive", "F"),
        ("print", "F"),
        ("low_digits", "F"),

        ("istream_impl", "G"),
        ("start", "G"),
        ("write_int_split", "G"),
        ("arg2", "G"),

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

        ("input_string_impl", "S"),

        ("input_line_impl", "T"),

        ("NULL", "0"),
        ("false", "0"),
        ("true", "1"),
        ("MAP_FAILED", "(void*)-1"),
        ("INVALID_HANDLE_VALUE", "(void*)-1"),
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
    s = re.sub(r"\s+", " ", s)
    s = s.strip()
    if s:
        s += "\n"
    return s

blazingio = "".join(whitespace(part) for part in re.split(r"(#.*)", blazingio))

# Remove whitespace after "#include"
blazingio = re.sub(r"#include\s+<", "#include<", blazingio)

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
blazingio = f"// DO NOT REMOVE THIS MESSAGE. The mess that follows is a minified build of\n// https://github.com/purplesyringa/blazingio. Refer to the repository for\n// a human-readable version and documentation.\n// Options: {config_str or '(none)'}\n{blazingio}\n// End of blazingio\n"

open("blazingio.min.hpp", "w").write(blazingio)

print("Wrote", len(blazingio), "bytes to blazingio.min.hpp")
