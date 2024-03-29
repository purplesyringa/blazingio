#include <cstdint>
#include <iostream>
#include <string>
#include BLAZINGIO

int main() {
    std::string s;
    uint64_t sum = 0;
    while (std::getline(std::cin, s)) {
        sum += s.size();
    }
    // Do not optimize out reading of s
    std::cout << sum << std::endl;
    return 0;
}
