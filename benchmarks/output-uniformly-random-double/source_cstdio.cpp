#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 500000; i++) {
        printf("%.12lf\n", (double)gen() * 4e290);
    }
    return 0;
}
