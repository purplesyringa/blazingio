#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 500000; i++) {
        int32_t rng = gen();
        double x = exp10(rng / 7158278.);
        if (rng & 1) {
            x = -x;
        }
        printf("%.12lg\n", x);
    }
    return 0;
}
