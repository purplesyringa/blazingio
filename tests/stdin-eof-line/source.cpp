#include <cassert>
#include <iostream>
#include <string>
#include BLAZINGIO

int main() {
    std::string s;
    assert(std::getline(std::cin, s));
    assert(s == std::string(4000, 'a'));
    assert(!std::getline(std::cin, s));
    return 0;
}
