#include <cassert>
#include <iostream>
#include <string>
#include BLAZINGIO

int main() {
    std::string s;
    assert(std::cin >> s);
    int n;
    assert(!(std::cin >> n));
    return 0;
}
