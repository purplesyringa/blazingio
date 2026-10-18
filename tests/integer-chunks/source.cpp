#include <cstdint>
#include <iostream>
#include BLAZINGIO

int main() {
    int count;
    std::cin >> count;
    while (count--) {
        int64_t value;
        std::cin >> value;
        std::cout << value << '\n';
    }
}
