#include <iostream>
#include <random>
#include BLAZINGIO

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 20000000; i++) {
        uint32_t rng = gen();
        int sign = rng & 1;
        rng >>= 1;
        int exp = rng & 31;
        int32_t x = gen() & ((1 << exp) - 1);
        if (sign) {
            x = -x;
        }
        std::cout << x << '\n';
    }
    return 0;
}
