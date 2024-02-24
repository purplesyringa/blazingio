#include <iostream>
#include BLAZINGIO

int main() {
    for (size_t i = 0; i < 20000000; i++) {
        std::cout << 'x';
    }
    return 0;
}
