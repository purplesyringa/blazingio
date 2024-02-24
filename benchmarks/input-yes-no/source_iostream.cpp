#include <iostream>

int main() {
    std::string s;
    uint64_t sum = 0;
    while (std::cin >> s) {
        sum += s == "YES";
    }
    // Do not optimize out reading of s
    std::cout << sum << std::endl;
    return 0;
}
