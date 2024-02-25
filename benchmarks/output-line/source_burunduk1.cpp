#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>

static const int buf_size = 4096;

static int write_pos = 0;
static char write_buf[buf_size];

inline void writeChar( int x ) {
    if (write_pos == buf_size)
        fwrite(write_buf, 1, buf_size, stdout), write_pos = 0;
    write_buf[write_pos++] = x;
}

inline void writeWord( const char *s ) {
    while (*s)
        writeChar(*s++);
}

int main() {
    char s[2048];
    std::minstd_rand gen;
    for (size_t j = 0; j < 2048; j++) {
        s[j] = gen() % (255 - 33 + 1) + 33;
    }
    for (size_t i = 0; i < 1000000; i++) {
        size_t length = gen() % 2001;
        s[length] = '\0';
        writeWord(s);
        writeChar('\n');
        s[length] = 'x';
    }
    return 0;
}
