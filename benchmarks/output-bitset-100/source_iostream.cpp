#include <bitset>
#include <iostream>

int main() {
    std::bitset<100> a;
    for (size_t i = 0; i < a.size(); i++) {
        a[i] = rand() % 2;
    }
    for (size_t i = 0; i < 500000; i++) {
        std::cout << a << '\n';
    }
    return 0;
}
