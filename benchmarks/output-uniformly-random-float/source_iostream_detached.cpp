#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::cout << std::setprecision(12);
    std::minstd_rand gen;
    for (size_t i = 0; i < 5000000; i++) {
        std::cout << (float)gen() * (float)4e10 << '\n';
    }
    return 0;
}
