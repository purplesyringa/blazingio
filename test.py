# Why can't I stop writing a new test harness in each project...

import yaml
import os
import subprocess
import sys

from common import CONFIG_OPTS


def iterate_config(config, props = []):
    if not config:
        yield props
        return
    (name, values), *tail = config
    for value in values:
        new_props = props
        opt = CONFIG_OPTS[f"{name}={value}"]
        if opt:
            new_props = new_props + [opt]
        yield from iterate_config(tail, new_props)


for test_name in os.listdir("tests"):
    print("Test", test_name)
    with open(f"tests/{test_name}/test.yaml") as f:
        manifest = yaml.safe_load(f)

    print("  Generating")
    with open("/tmp/blazingio-test", "wb") as f:
        subprocess.run([sys.executable, f"tests/{test_name}/gen.py"], stdout=f, check=True)
    with open("/tmp/blazingio-test", "rb") as f:
        test = f.read()

    config = list(manifest.get("config", {}).items())
    for props in iterate_config(config):
        print(f"  Props {' '.join(props) if props else '(none)'}")

        if manifest["type"] == "round-trip":
            print("    Compiling")
            subprocess.run(["g++", f"tests/{test_name}/source.cpp", "-iquote", "."] + [f"-D{prop}" for prop in props], check=True)
            print("    Running")
            with open("/tmp/blazingio-test", "rb") as f:
                with open("/tmp/blazingio-out", "wb") as f2:
                    subprocess.run(["./a.out"], stdin=f, stdout=f2, check=True)
                with open("/tmp/blazingio-out", "rb") as f2:
                    assert test == f2.read()
        elif manifest["type"] == "compare-std":
            print("    Compiling with blazingio")
            subprocess.run(["g++", f"tests/{test_name}/source.cpp", "-iquote", ".", "-o", "a.out.blazingio"] + [f"-D{prop}" for prop in props], check=True)
            print("    Compiling without blazingio")
            subprocess.run(["g++", f"tests/{test_name}/source.cpp", "-iquote", ".", "-o", "a.out.std", "-DNO_BLAZINGIO"], check=True)
            print("    Running with blazingio")
            with open("/tmp/blazingio-test", "rb") as f:
                with open("/tmp/blazingio-out-blazingio", "wb") as f2:
                    subprocess.run(["./a.out.blazingio"], stdin=f, stdout=f2, check=True)
            print("    Running with std")
            with open("/tmp/blazingio-test", "rb") as f:
                with open("/tmp/blazingio-out-std", "wb") as f2:
                    subprocess.run(["./a.out.std"], stdin=f, stdout=f2, check=True)
            with open("/tmp/blazingio-out-blazingio", "rb") as f:
                with open("/tmp/blazingio-out-std", "rb") as f2:
                    assert f.read() == f2.read()
