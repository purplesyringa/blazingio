#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>

int main() {
    std::cout << std::setprecision(12);
    std::minstd_rand gen;
    for (size_t i = 0; i < 5000000; i++) {
        std::cout << (float)gen() * (float)4e10 << '\n';
    }
    return 0;
}
