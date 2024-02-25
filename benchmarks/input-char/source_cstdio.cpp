#include <cstdint>
#include <cstdio>
#include <inttypes.h>

int main() {
    char c;
    uint64_t sum = 0;
    while (scanf("%c", &c) > 0) {
        sum += c;
    }
    // Do not optimize out reading of c
    printf("%" PRIu64 "\n", sum);
    return 0;
}
