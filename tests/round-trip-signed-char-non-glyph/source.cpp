#include <iostream>
#include "blazingio.hpp"

template<typename T>
void round_trip() {
    size_t count;
    std::cin >> count;
    std::cout << count << std::endl;

    for (size_t i = 0; i < count; i++) {
        T n;
        std::cin >> n;
        std::cout << n << ' ';
    }
    std::cout << std::endl;
}

int main() {
    round_trip<int8_t>();
    round_trip<uint8_t>();
    return 0;
}
