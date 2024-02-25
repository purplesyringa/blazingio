#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 20000000; i++) {
        printf("%hd\n", static_cast<int16_t>(gen()));
    }
    return 0;
}
