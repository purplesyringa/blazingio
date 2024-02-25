#include <cstdint>
#include <cstdio>
#include <cstring>
#include <inttypes.h>

int main() {
    char s[2048];
    uint64_t sum = 0;
    while (scanf("%s", s) == 1) {
        sum += strlen(s);
    }
    // Do not optimize out reading of s
    printf("%" PRIu64 "\n", sum);
    return 0;
}
