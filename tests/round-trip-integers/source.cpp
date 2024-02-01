#include <iostream>
#include "blazingio.hpp"

template<typename T>
void round_trip() {
    size_t count;
    std::cin >> count;
    std::cout << count << std::endl;

    uint64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        T n;
        std::cin >> n;
        sum += n;
        std::cout << n << ' ';
    }
    std::cout << std::endl << sum << std::endl;
    std::cin >> sum;
}

int main() {
    round_trip<int16_t>();
    round_trip<uint16_t>();
    round_trip<int32_t>();
    round_trip<uint32_t>();
    round_trip<int64_t>();
    round_trip<uint64_t>();
    return 0;
}
