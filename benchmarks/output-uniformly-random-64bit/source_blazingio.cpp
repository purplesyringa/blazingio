#include <cstdint>
#include <iostream>
#include <random>
#include BLAZINGIO

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 10000000; i++) {
        std::cout << ((static_cast<int64_t>(gen()) << 32) | gen()) << '\n';
    }
    return 0;
}
