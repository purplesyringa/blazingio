#include <iostream>
#include <random>
#include BLAZINGIO

char s[2048];

__attribute__((target("ssse3")))
void generate(size_t length, size_t i) {
    // Generate the string as fast as possible to avoid its generation being counted in
    // benchmark
    __m128i a = _mm_set_epi16(
        (i + 7) * 26, (i + 6) * 26, (i + 5) * 26, (i + 4) * 26,
        (i + 3) * 26, (i + 2) * 26, (i + 1) * 26, (i + 0) * 26
    );
    __m128i b = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 15, 13, 11, 9, 7, 5, 3, 1);
    __m128i c = _mm_set1_epi16(26);
    for (size_t j = 0; j < length; j += 8) {
        int64_t x = _mm_cvtsi128_si64(_mm_shuffle_epi8(a, b)) + 0x6161616161616161;
        memcpy(s + j, &x, 8);
        a = _mm_add_epi16(a, c);
    }
}

int main() {
    std::minstd_rand gen;
    for (size_t i = 0; i < 100000; i++) {
        size_t length = gen() % 2001;
        generate(length, i);
        std::cout << std::string_view(s, length) << '\n';
    }
    return 0;
}
