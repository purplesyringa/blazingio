#include <iostream>
#include "blazingio.hpp"

int main() {
    uint64_t sum = 0;
    for (int i = 0; i < 10000; i++) {
        char c;
        std::cin >> c;
        std::cout << c;
        sum += (uint8_t)c;
    }
    std::cout << sum << std::endl;
    return 0;
}
