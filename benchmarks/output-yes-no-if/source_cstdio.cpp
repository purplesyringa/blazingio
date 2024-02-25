#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 10000000; i += 32) {
        uint32_t n = gen();
        for (size_t j = 0; j < 32; j++) {
            if ((n >> j) & 1) {
                printf("YES");
            } else {
                printf("NO");
            }
            printf(" ");
        }
    }
    return 0;
}
