#include <iostream>
#include BLAZINGIO

template<typename T>
void round_trip() {
    uint64_t sum = 0;
    for (int i = 0; i < 10000; i++) {
        T c;
        std::cin >> c;
        std::cout << c;
        sum += (uint8_t)c;
    }
    std::cout << sum << std::endl;
    std::cin >> sum;
}

int main() {
    round_trip<int8_t>();
    round_trip<uint8_t>();
    return 0;
}
