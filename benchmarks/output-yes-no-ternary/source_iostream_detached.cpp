#include <iostream>
#include <random>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::minstd_rand gen;
    for (size_t i = 0; i < 20000000; i += 32) {
        uint32_t n = gen();
        for (size_t j = 0; j < 32; j++) {
            std::cout << (((n >> j) & 1) ? "YES" : "NO") << ' ';
        }
    }
    return 0;
}
