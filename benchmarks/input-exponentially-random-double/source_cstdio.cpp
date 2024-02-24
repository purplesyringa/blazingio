#include <cstdint>
#include <cstdio>

int main() {
    double n;
    double sum = 0;
    while (scanf("%lf", &n) > 0) {
        sum += n;
    }
    // Do not optimize out reading of n
    printf("%lf\n", sum);
    return 0;
}
