#include <cstdint>
#include <iostream>
#include BLAZINGIO

int main() {
    char c;
    uint64_t sum = 0;
    while (std::cin >> c) {
        sum += c;
    }
    // Do not optimize out reading of c
    std::cout << sum << std::endl;
    return 0;
}
