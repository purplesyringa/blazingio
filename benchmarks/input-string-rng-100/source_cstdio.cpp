#include <cstdint>
#include <cstdio>
#include <cstring>

int main() {
    char s[2048];
    uint64_t sum = 0;
    while (fgets(s, 2048, stdin)) {
        sum += strlen(s);
    }
    // Do not optimize out reading of s
    printf("%lu\n", sum);
    return 0;
}