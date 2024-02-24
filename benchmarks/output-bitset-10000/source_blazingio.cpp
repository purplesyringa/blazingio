#include <bitset>
#include <iostream>
#include BLAZINGIO

int main() {
    std::bitset<10000> a;
    for (size_t i = 0; i < a.size(); i++) {
        a[i] = rand() % 2;
    }
    for (size_t i = 0; i < 5000; i++) {
        std::cout << a << '\n';
    }
    return 0;
}
