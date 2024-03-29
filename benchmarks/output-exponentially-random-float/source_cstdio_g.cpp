#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 2000000; i++) {
        int32_t rng = gen();
        float x = (float)exp(rng / 46684427.);
        if (rng & 1) {
            x = -x;
        }
        printf("%.12g\n", x);
    }
    return 0;
}
