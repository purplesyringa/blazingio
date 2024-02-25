#include <cstdint>
#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    char c;
    uint64_t sum = 0;
    while (std::cin >> c) {
        sum += c;
    }
    // Do not optimize out reading of c
    std::cout << sum << std::endl;
    return 0;
}
