#include <cmath>
#include <iostream>
#include <random>
#include BLAZINGIO

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 2000000; i++) {
        int32_t rng = gen();
        float x = (float)exp(rng / 46684427.);
        if (rng & 1) {
            x = -x;
        }
        std::cout << x << '\n';
    }
    return 0;
}
