# Why can't I stop writing a new test harness in each project...

import json
import yaml
import os
import platform
import subprocess
import sys

try:
    import resource
    is_windows = False
except ModuleNotFoundError:
    import win32process
    is_windows = True


tmp = os.environ.get("TEMP", "/tmp")

if "--bench" in sys.argv:
    index = sys.argv.index("--bench")
    benchmark_list = sys.argv[index + 1:]
    del sys.argv[index:]
    bench = True
else:
    bench = False

disable_sanitizers = bench

def compile(source, target, blazingio):
    subprocess.run(
        gcc
        + [source, "-o", target, "-iquote", ".", f"-DBLAZINGIO=\"{blazingio}\"", "-std=c++17", "-O2"]
        + ["-Wall", "-Wextra", "-Wconversion", "-Wno-sign-conversion", "-pedantic-errors", "-Werror"]
        + ([] if disable_sanitizers else ["-fsanitize=address,pointer-compare,pointer-subtract,undefined"]),
        check=True
    )

if len(sys.argv) >= 2 and sys.argv[1] in ("--cross", "--cross-windows"):
    arch = sys.argv[2]
    gcc_arch = "i686" if arch == "i386" else arch
    if sys.argv[1] == "--cross":
        gcc = [f"{gcc_arch}-linux-gnu-g++"]
    else:
        gcc = [f"{gcc_arch}-w64-mingw32-g++", "-static"]
        disable_sanitizers = True
elif len(sys.argv) >= 2 and sys.argv[1] == "--msvc":
    arch = sys.argv[2]
    def compile(source, target, blazingio):
        subprocess.run(
            [f"cl", source, "/I.", f"/DBLAZINGIO=\"{blazingio}\"", f"/Fe{target}", "/std:c++17", "/O2", "/EHsc", "/nologo", "/W2", "/WX"]
            + ([] if disable_sanitizers else ["/fsanitize=address", "/Zi"]),
            check=True
        )
    if "GITHUB_RUN_ID" in os.environ:
        os.environ["CPP"] = "msys2 -c \"exec cpp $*\" cpp"
    else:
        os.environ["CPP"] = "C:\\msys64\\usr\\bin\\bash.exe -l -c \"exec cpp $*\" cpp"
    os.environ["MSYSTEM"] = "UCRT64"
else:
    arch = platform.machine()
    if arch == "AMD64":
        arch = "x86_64"
    elif arch == "arm64":
        arch = "aarch64"
    if len(sys.argv) >= 2 and sys.argv[1] == "--clang":
        gcc = ["clang++"]
    else:
        gcc = ["g++"]
    if platform.system() == "Windows":
        disable_sanitizers = True


def iterate_config(config, props = []):
    if not config:
        if "interactive=y" in props:
            yield props, True
        yield props, False
        return
    (name, values), *tail = config
    if not isinstance(values, list):
        values = [values]
    for value in values:
        new_props = props + [f"{name}={value}"]
        if name == "architecture" and arch not in value:
            continue
        yield from iterate_config(tail, new_props)


def run(name, input, output, use_pipe):
    if not is_windows:
        rusage = resource.getrusage(resource.RUSAGE_CHILDREN)
        cpu_time_before = rusage.ru_utime + rusage.ru_stime
    if use_pipe:
        with open(input, "rb") as f_stdin:
            input = f_stdin.read()
        proc = subprocess.Popen(
            [name],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE
        )
        stdout, _ = proc.communicate(input)
        assert proc.poll() == 0
        with open(output, "wb") as f_stdout:
            f_stdout.write(stdout)
    else:
        with open(input, "rb") as f_stdin:
            with open(output, "wb") as f_stdout:
                proc = subprocess.Popen(
                    [name],
                    stdin=f_stdin,
                    stdout=f_stdout
                )
                assert proc.wait() == 0
    if is_windows:
        stat = win32process.GetProcessTimes(proc._handle)
        return (stat["KernelTime"] + stat["UserTime"]) / 1e7
    rusage = resource.getrusage(resource.RUSAGE_CHILDREN)
    cpu_time_after = rusage.ru_utime + rusage.ru_stime
    return cpu_time_after - cpu_time_before


def assert_equal_files(a, b):
    subprocess.run(["git", "diff", "--no-index", "--ignore-cr-at-eol", a, b], check=True)


if bench:
    print("Minimizing")
    subprocess.run([sys.executable, "minimize.py"], stdout=subprocess.DEVNULL, check=True)

    log = {}

    for benchmark_name in benchmark_list or os.listdir("benchmarks"):
        print("Benchmark", benchmark_name)

        generator = f"benchmarks/{benchmark_name}/gen.py"
        if os.path.exists(generator):
            print("  Generating")
            with open(f"{tmp}/blazingio-test", "wb") as f:
                subprocess.run([sys.executable, generator], stdout=f, check=True)
        else:
            with open(f"{tmp}/blazingio-test", "wb") as f:
                pass
        with open(f"{tmp}/blazingio-test", "rb") as f:
            test = f.read()

        for file in os.listdir(f"benchmarks/{benchmark_name}"):
            if file.startswith("source_"):
                impl_name = file[7:].partition(".")[0]
                print(f"  Compiling {file}")
                compile(f"benchmarks/{benchmark_name}/{file}", "a.out", "blazingio.min.hpp")

                for use_pipe in (False, True):
                    print("    Pipe I/O" if use_pipe else "    File I/O")
                    times = []
                    for _ in range(10):
                        times.append(run("./a.out", f"{tmp}/blazingio-test", f"{tmp}/blazingio-out", use_pipe))
                    tm = sum(times[2:-2]) / 6
                    print(f"      Took {tm:.3} s")
                    key = f"{benchmark_name}/{impl_name}/{'pipe' if use_pipe else 'file'}"
                    log[key] = tm

    with open("benchmark-results.json", "w") as f:
        json.dump(log, f)
else:
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
        for props, use_pipe in iterate_config(config):
            print(f"  Props {' '.join(props)}, {'pipe' if use_pipe else 'file'}")

            print("    Minimizing")
            subprocess.run([sys.executable, "minimize.py", "--override"] + props, stdout=subprocess.DEVNULL, check=True)

            if manifest["type"] == "round-trip":
                print("    Compiling")
                compile(f"tests/{test_name}/source.cpp", "a.out", "blazingio.min.hpp")
                print("    Running")
                run("./a.out", f"{tmp}/blazingio-test", f"{tmp}/blazingio-out", use_pipe)
                assert_equal_files(f"{tmp}/blazingio-test", f"{tmp}/blazingio-out")
            elif manifest["type"] == "compare-std":
                print("    Compiling with blazingio")
                compile(f"tests/{test_name}/source.cpp", "a.out.blazingio", "blazingio.min.hpp")
                print("    Compiling without blazingio")
                compile(f"tests/{test_name}/source.cpp", "a.out.std", "empty.hpp")
                print("    Running with blazingio")
                run("./a.out.blazingio", f"{tmp}/blazingio-test", f"{tmp}/blazingio-out-blazingio", use_pipe)
                print("    Running with std")
                run("./a.out.std", f"{tmp}/blazingio-test", f"{tmp}/blazingio-out-std", use_pipe)
                if "approx" in manifest:
                    approx = manifest["approx"]
                    with open(f"{tmp}/blazingio-out-blazingio", "rb") as f:
                        with open(f"{tmp}/blazingio-out-std", "rb") as f2:
                            blazingio = f.read().split()
                            std = f2.read().replace(b"\r\n", b"\n").split()
                            assert len(blazingio) == len(std)
                            for a, b in zip(blazingio, std):
                                a_n = float(a)
                                b_n = float(b)
                                assert abs(a_n - b_n) / max(1, min(abs(a_n), abs(b_n))) < approx, (a, b)
                else:
                    assert_equal_files(f"{tmp}/blazingio-out-blazingio", f"{tmp}/blazingio-out-std")
            elif manifest["type"] == "exit-code":
                print("    Compiling")
                compile(f"tests/{test_name}/source.cpp", "a.out", "blazingio.min.hpp")
                print("    Running")
                run("./a.out", f"{tmp}/blazingio-test", f"{tmp}/blazingio-out", use_pipe)
