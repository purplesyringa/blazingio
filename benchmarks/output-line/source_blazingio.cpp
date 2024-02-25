#include <cstdint>
#include <iostream>
#include <random>
#include BLAZINGIO

int main() {
    char s[2048];
    std::minstd_rand gen;
    for (size_t j = 0; j < 2048; j++) {
        s[j] = gen() % (255 - 33 + 1) + 33;
    }
    for (size_t i = 0; i < 100000; i++) {
        size_t length = gen() % 2001;
        std::cout << std::string_view(s, length) << '\n';
    }
    return 0;
}
