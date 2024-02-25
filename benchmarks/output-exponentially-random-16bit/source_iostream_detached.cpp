#include <iostream>
#include <random>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::minstd_rand gen;
    for (size_t i = 0; i < 20000000; i++) {
        uint32_t rng = gen();
        int sign = rng & 1;
        rng >>= 1;
        int exp = rng & 15;
        rng >>= 4;
        int16_t x = rng & ((1 << exp) - 1);
        if (sign) {
            x = -x;
        }
        std::cout << x << '\n';
    }
    return 0;
}
