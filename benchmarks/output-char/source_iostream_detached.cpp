#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    for (size_t i = 0; i < 40000000; i++) {
        std::cout << 'x';
    }
    return 0;
}
