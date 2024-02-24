#include <iostream>
#include <random>
#include BLAZINGIO

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 500000; i++) {
        int32_t rng = gen();
        float x = exp10(rng / 107374182.);
        if (rng & 1) {
            x = -x;
        }
        std::cout << x << '\n';
    }
    return 0;
}
