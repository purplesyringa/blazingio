#include <cstdint>
#include <iostream>
#include <string>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::string s;
    uint64_t sum = 0;
    while (std::getline(std::cin, s)) {
        sum += s.size();
    }
    // Do not optimize out reading of s
    std::cout << sum << std::endl;
    return 0;
}
