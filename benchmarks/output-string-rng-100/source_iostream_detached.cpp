#include <iostream>
#include <random>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    char s[128];
    std::minstd_rand gen;
    for (size_t j = 0; j < 128; j += 8) {
        s[j] = gen() % (255 - 33 + 1) + 33;
    }
    uint64_t n = 1;
    for (size_t i = 0; i < 10000000; i++) {
        size_t length = (n >> 32) % 99 + 1;
        std::cout << std::string_view(s, length) << ' ';
        n *= 0x7432974328912321;
    }
    return 0;
}
