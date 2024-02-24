#include <iostream>
#include <random>
#include BLAZINGIO

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 5000000; i++) {
        std::cout << static_cast<int32_t>(gen()) << '\n';
    }
    return 0;
}
