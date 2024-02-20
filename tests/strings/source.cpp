#include <iostream>
#include BLAZINGIO

int main() {
    std::cout
        << "Hello, "
        << std::string_view("world! ")
        << std::flush
        << std::string("This is amazing")
        << '.'
        << std::endl;
    std::cout.flush();
    return 0;
}
