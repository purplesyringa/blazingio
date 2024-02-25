#include <iostream>
#include <random>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::minstd_rand gen;
    for (size_t i = 0; i < 20000000; i++) {
        std::cout << static_cast<int16_t>(gen()) << '\n';
    }
    return 0;
}
