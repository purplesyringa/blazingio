#include <iostream>
#include <random>

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 5000000; i++) {
        std::cout << ((static_cast<int64_t>(gen()) << 32) | gen()) << '\n';
    }
    return 0;
}
