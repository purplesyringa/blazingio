#include <iostream>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 10000000; i += 32) {
        uint32_t n = gen();
        for (size_t j = 0; j < 32; j++) {
            if ((n >> j) & 1) {
                std::cout << "YES";
            } else {
                std::cout << "NO";
            }
            std::cout << ' ';
        }
    }
    return 0;
}
