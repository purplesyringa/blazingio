#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::cout << std::setprecision(12);
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
