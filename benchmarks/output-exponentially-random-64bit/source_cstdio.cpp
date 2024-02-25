#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 20000000; i++) {
        uint32_t rng = gen();
        int sign = rng & 1;
        rng >>= 1;
        int exp = rng & 63;
        int32_t x = (((uint64_t)gen() << 32) | gen()) & ((1ULL << exp) - 1);
        if (sign) {
            x = -x;
        }
        printf("%hd\n", x);
    }
    return 0;
}
