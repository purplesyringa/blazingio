#include <cstdint>
#include <cstdio>
#include <inttypes.h>

int main() {
    int64_t n;
    uint64_t sum = 0;
    while (scanf("%" PRId64, &n) > 0) {
        sum += n;
    }
    // Do not optimize out reading of n
    printf("%" PRIu64 "\n", sum);
    return 0;
}
