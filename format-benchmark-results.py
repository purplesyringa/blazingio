import json
import sys


with open(sys.argv[1]) as f:
    results = json.load(f)

benchmark_names = sorted({key.split("/")[0] for key in results.keys()})
implementation_names = [
    "blazingio",
    "iostream_detached",
    "cstdio",
    "cstdio_g",
    "burunduk1"
]

print("", end="\t")
for implementation in implementation_names:
    print(implementation, end="\t")
print()

for benchmark in benchmark_names:
    print(benchmark, end="\t")
    for implementation in implementation_names:
        print(results.get(f"{benchmark}/{implementation}/{sys.argv[2]}", ""), end="\t")
    print()

