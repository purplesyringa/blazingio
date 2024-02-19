CONFIG_OPTS = {
    "lut=n": ("", ""),
    "lut=y": ("LUT", "l"),
    "char_with_sign_is_glyph=n": ("", ""),
    "char_with_sign_is_glyph=y": ("CHAR_WITH_SIGN_IS_GLYPH", "c"),
    "bitset=n": ("", ""),
    "bitset=y": ("BITSET", "b"),
    "float=n": ("", ""),
    "float=y": ("FLOAT", "f"),
    "complex=n": ("", ""),
    "complex=y": ("COMPLEX", "o"),
    "interactive=n": ("", ""),
    "interactive=y": ("INTERACTIVE", "i"),
    "stdin_eof=n": ("", ""),
    "stdin_eof=y": ("STDIN_EOF", "e"),
    "late_binding=n": ("", ""),
    "late_binding=y": ("LATE_BINDING", "d"),
    "cerr=n": ("", ""),
    "cerr=y": ("CERR", "r"),
    "hoist_globals_on_interactive_input=n": ("", ""),
    "hoist_globals_on_interactive_input=y": ("HOIST_GLOBALS_ON_INTERACTIVE_INPUT", "h"),
}

ARCHITECTURES = {
    "x86_64": ({"sse4.1": "s", "avx2": "a"}, "X"),
    "aarch64": ({"neon": "n"}, "A"),
}

OSES = {
    "windows": "W",
    "linux": "L",
    "macos": "M",
}
