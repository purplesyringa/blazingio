#include <cmath>
#include <iostream>
#include <random>
#include BLAZINGIO

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 5000000; i++) {
        std::cout << (float)gen() * (float)4e10 << '\n';
    }
    return 0;
}
