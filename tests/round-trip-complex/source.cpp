#include <complex>
#include <iostream>
#include BLAZINGIO

int main() {
    size_t n;
    std::cin >> n;
    std::cout << n << std::endl;

    std::complex<long long> sum = 0;

    for (size_t i = 0; i < n; i++) {
        std::complex<int> x;
        std::cin >> x;
        std::cout << x << std::endl;
        sum += x;
    }

    std::cout << sum << std::endl;
    return 0;
}
