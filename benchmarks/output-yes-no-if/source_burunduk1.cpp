#include <cstdint>
#include <cstdio>
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
    std::minstd_rand gen;
    for (size_t i = 0; i < 20000000; i += 32) {
        uint32_t n = gen();
        for (size_t j = 0; j < 32; j++) {
            if ((n >> j) & 1) {
                writeWord("YES");
            } else {
                writeWord("NO");
            }
            writeChar(' ');
        }
    }
    return 0;
}
