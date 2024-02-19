# Why can't I stop writing a new test harness in each project...

import yaml
import os
import platform
import subprocess
import sys


tmp = "/tmp"

if len(sys.argv) >= 2 and sys.argv[1] == "--cross":
    arch = sys.argv[2]
    def compile(source, target, blazingio):
        subprocess.run([f"{arch}-linux-gnu-g++", source, "-o", target, "-iquote", ".", f"-DBLAZINGIO=\"{blazingio}\""], check=True)
elif len(sys.argv) >= 2 and sys.argv[1] == "--cross-windows":
    arch = platform.machine()
    def compile(source, target, blazingio):
        subprocess.run([f"{arch}-w64-mingw32-g++", "-static", source, "-o", target, "-iquote", ".", f"-DBLAZINGIO=\"{blazingio}\""], check=True)
elif len(sys.argv) >= 2 and sys.argv[1] == "--msvc":
    arch = platform.machine()
    if arch == "AMD64":
        arch = "x86_64"
    def compile(source, target, blazingio):
        subprocess.run([f"cl", source, "/I.", f"/DBLAZINGIO=\"{blazingio}\"", f"/Fe{target}", "/std:c++17", "/EHsc", "/nologo"], check=True)
    tmp = os.environ["TEMP"]
    os.environ["CPP"] = "C:\\msys64\\usr\\bin\\bash.exe -l -c \"exec cpp $*\" cpp"
    os.environ["MSYSTEM"] = "UCRT64"
else:
    arch = platform.machine()
    def compile(source, target, blazingio):
        subprocess.run(["g++", source, "-o", target, "-iquote", ".", f"-DBLAZINGIO=\"{blazingio}\""], check=True)


def iterate_config(config, props = []):
    if not config:
        yield props
        return
    (name, values), *tail = config
    if not isinstance(values, list):
        values = [values]
    for value in values:
        new_props = props + [f"{name}={value}"]
        if name == "architecture" and arch not in value:
            continue
        yield from iterate_config(tail, new_props)


for test_name in os.listdir("tests"):
    print("Test", test_name)
    with open(f"tests/{test_name}/test.yaml") as f:
        manifest = yaml.safe_load(f)

    print("  Generating")
    with open(f"{tmp}/blazingio-test", "wb") as f:
        subprocess.run([sys.executable, f"tests/{test_name}/gen.py"], stdout=f, check=True)
    with open(f"{tmp}/blazingio-test", "rb") as f:
        test = f.read()

    config = list(manifest.get("config", {}).items())
    for props in iterate_config(config):
        print(f"  Props {' '.join(props)}")

        print("    Minimizing")
        subprocess.run([sys.executable, "minimize.py", "--override"] + props, stdout=subprocess.DEVNULL, check=True)

        if manifest["type"] == "round-trip":
            print("    Compiling")
            compile(f"tests/{test_name}/source.cpp", "a.out", "blazingio.min.hpp")
            print("    Running")
            with open(f"{tmp}/blazingio-test", "rb") as f:
                with open(f"{tmp}/blazingio-out", "wb") as f2:
                    subprocess.run(["./a.out"], stdin=f, stdout=f2, check=True)
                with open(f"{tmp}/blazingio-out", "rb") as f2:
                    assert test.replace(b"\r\n", b"\n") == f2.read()
        elif manifest["type"] == "compare-std":
            print("    Compiling with blazingio")
            compile(f"tests/{test_name}/source.cpp", "a.out.blazingio", "blazingio.min.hpp")
            print("    Compiling without blazingio")
            compile(f"tests/{test_name}/source.cpp", "a.out.std", "empty.hpp")
            print("    Running with blazingio")
            with open(f"{tmp}/blazingio-test", "rb") as f:
                with open(f"{tmp}/blazingio-out-blazingio", "wb") as f2:
                    subprocess.run(["./a.out.blazingio"], stdin=f, stdout=f2, check=True)
            print("    Running with std")
            with open(f"{tmp}/blazingio-test", "rb") as f:
                with open(f"{tmp}/blazingio-out-std", "wb") as f2:
                    subprocess.run(["./a.out.std"], stdin=f, stdout=f2, check=True)
            with open(f"{tmp}/blazingio-out-blazingio", "rb") as f:
                with open(f"{tmp}/blazingio-out-std", "rb") as f2:
                    assert f.read() == f2.read().replace(b"\r\n", b"\n")
        elif manifest["type"] == "exit-code":
            print("    Compiling")
            compile(f"tests/{test_name}/source.cpp", "a.out", "blazingio.min.hpp")
            print("    Running")
            with open(f"{tmp}/blazingio-test", "rb") as f:
                subprocess.run(["./a.out"], stdin=f, check=True)
