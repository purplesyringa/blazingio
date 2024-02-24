#include <iostream>
#include BLAZINGIO

int main() {
    int16_t n;
    uint64_t sum = 0;
    while (std::cin >> n) {
        sum += n;
    }
    // Do not optimize out reading of n
    std::cout << sum << std::endl;
    return 0;
}
