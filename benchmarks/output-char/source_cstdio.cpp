#include <cstdio>

int main() {
    for (size_t i = 0; i < 40000000; i++) {
        printf("%c", 'x');
    }
    return 0;
}
