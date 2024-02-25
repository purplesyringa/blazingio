#include <cstdint>
#include <cstdio>
#include <inttypes.h>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 5000000; i++) {
        printf("%" PRId64 "\n", (static_cast<int64_t>(gen()) << 32) | gen());
    }
    return 0;
}
