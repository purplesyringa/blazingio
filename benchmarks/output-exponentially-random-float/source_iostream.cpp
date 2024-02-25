#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>

int main() {
    std::cout << std::setprecision(12);
    std::minstd_rand gen;
    for (size_t i = 0; i < 500000; i++) {
        int32_t rng = gen();
        float x = (float)exp(rng / 46684427.);
        if (rng & 1) {
            x = -x;
        }
        std::cout << x << '\n';
    }
    return 0;
}
