#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>

int main() {
    std::cout << std::setprecision(12);
    std::minstd_rand gen;
    for (size_t i = 0; i < 500000; i++) {
        std::cout << (double)gen() * 4e290 << '\n';
    }
    return 0;
}
