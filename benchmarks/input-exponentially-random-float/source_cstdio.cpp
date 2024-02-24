#include <cstdint>
#include <cstdio>

int main() {
    float n;
    float sum = 0;
    while (scanf("%f", &n) > 0) {
        sum += n;
    }
    // Do not optimize out reading of n
    printf("%f\n", sum);
    return 0;
}
