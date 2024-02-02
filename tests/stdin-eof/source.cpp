#include <iostream>
#include "blazingio.hpp"

int main() {
    int count = 0;
    int n;
    while (std::cin >> n) {
        count++;
        std::cout << n << std::endl;
    }
    std::cout << "total " << count << std::endl;
    return 0;
}
