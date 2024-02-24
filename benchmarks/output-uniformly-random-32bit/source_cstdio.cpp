#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 5000000; i++) {
        printf("%d\n", static_cast<int32_t>(gen()));
    }
    return 0;
}
