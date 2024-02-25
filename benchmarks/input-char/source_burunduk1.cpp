#include <cstdio>
#include <iostream>

static const int buf_size = 4096;

inline int getChar() {
    static unsigned char buf[buf_size];
    static int len = 0, pos = 0;
    if (pos == len)
        pos = 0, len = fread(buf, 1, buf_size, stdin);
    if (pos == len)
        return -1;
    return buf[pos++];
}

inline int readChar() {
    int c = getChar();
    while (c > 0 && c <= 32)
        c = getChar();
    return c;
}

int main() {
    uint64_t sum = 0;
    int c;
    while ((c = readChar()) != -1) {
        sum += c;
    }
    // Do not optimize out reading of c
    std::cout << sum << std::endl;
    return 0;
}
