#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    char s[2048];
    std::minstd_rand gen;
    for (size_t j = 0; j < 2048; j++) {
        s[j] = gen() % (255 - 33 + 1) + 33;
    }
    uint64_t n = 1;
    for (size_t i = 0; i < 10000000; i++) {
        size_t length = (n >> 32) % 99 + 1;
        printf("%.*s ", static_cast<int>(length), s);
        n *= 0x7432974328912321;
    }
    return 0;
}
