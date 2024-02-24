#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::string s;
    uint64_t sum = 0;
    while (std::cin >> s) {
        sum += s == "YES";
    }
    // Do not optimize out reading of s
    std::cout << sum << std::endl;
    return 0;
}
