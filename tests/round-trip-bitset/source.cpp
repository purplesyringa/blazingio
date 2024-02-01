#include <iostream>
#include "blazingio.hpp"

template<size_t N>
void round_trip() {
    std::bitset<N> bs;
    std::cin >> bs;

    // Make sure we don't corrupt memory
    std::bitset<N> bs2 = bs;
    std::cout << bs2 << std::endl;

    uint64_t hash = 0;
    for (size_t i = 0; i < N; i++) {
        hash *= 57;
        hash += bs2[i];
    }

    std::cout << hash << std::endl;
    std::cin >> hash;
}

int main() {
    round_trip<0>();
    round_trip<1>();
    round_trip<15>();
    round_trip<32>();
    round_trip<64>();
    round_trip<128>();
    round_trip<256>();
    round_trip<300>();
    round_trip<5000>();
    return 0;
}
