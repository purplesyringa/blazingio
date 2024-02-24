#include <bitset>
#include <iostream>
#include BLAZINGIO

int main() {
    std::bitset<1000> a;
    size_t popcnt = 0;
    while (std::cin >> a) {
        popcnt += a.count();
    }
    // Do not optimize out reading of a
    std::cout << popcnt << std::endl;
    return 0;
}
