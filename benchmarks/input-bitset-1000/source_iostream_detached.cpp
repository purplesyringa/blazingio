#include <bitset>
#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::bitset<1000> a;
    size_t popcnt = 0;
    while (std::cin >> a) {
        popcnt += a.count();
    }
    // Do not optimize out reading of a
    std::cout << popcnt << std::endl;
    return 0;
}
