#include <bitset>
#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    std::bitset<10000> a;
    for (size_t i = 0; i < a.size(); i++) {
        a[i] = rand() % 2;
    }
    for (size_t i = 0; i < 40000; i++) {
        std::cout << a << '\n';
    }
    return 0;
}
