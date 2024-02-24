#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(0);
    std::cin.tie(0);

    double n;
    double sum = 0;
    while (std::cin >> n) {
        sum += n;
    }
    // Do not optimize out reading of n
    std::cout << sum << std::endl;
    return 0;
}
