#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 20000000; i += 32) {
        uint32_t n = gen();
        for (size_t j = 0; j < 32; j++) {
            printf("%s ", ((n >> j) & 1) ? "YES" : "NO");
        }
    }
    return 0;
}
