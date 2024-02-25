#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 5000000; i++) {
        printf("%.12g\n", (float)gen() * (float)4e10);
    }
    return 0;
}
