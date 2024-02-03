#   ifndef NO_BLAZINGIO

#include <array>
#   ifdef BITSET
#include <bitset>
#   endif
#   ifdef COMPLEX
#include <complex>
#   endif
#include <cstring>
#include <fcntl.h>
#   if defined(AVX2) || defined(SSE41)
#include <immintrin.h>
#   endif
#include <limits>
#include <sys/mman.h>
#   ifndef MINIMIZE
#include <unistd.h>
#   endif

#   ifdef AVX2
#define SIMD __attribute__((target("avx2")))
#   elif defined(SSE41)
#define SIMD __attribute__((target("sse4.1")))
#   else
#   define SIMD
#   endif

#define ensure(x) if (!(x)) abort();

namespace blazingio {

using namespace std;

struct UninitChar { UninitChar& operator=(UninitChar) { return *this; } };

struct NonAliasingChar {
    enum class Inner : char {};
    Inner c;
    NonAliasingChar& operator=(char x) {
        c = Inner{x};
        return *this;
    }
    operator char() {
        return (char)c;
    }
};

const long BIG = 0x1000000000
#   ifdef BITSET
, ONE_BYTES = -1ULL / 255
#   if !defined(AVX2) && !defined(SSE41)
, BITSET_SHIFT = 0x8040201008040201
#   endif
#   endif
;

struct line_t {
    std::string& value;
};

struct blazingio_istream {
    off_t file_size;
    char* base;
    NonAliasingChar* ptr;

    explicit blazingio_istream() {
        file_size = lseek(STDIN_FILENO, 0, SEEK_END);
        ensure(file_size != -1)
        // Round to page size.
        (file_size += 4095) &= -4096;
        base = (char*)mmap(NULL, file_size + 4096, PROT_READ, MAP_PRIVATE, STDIN_FILENO, 0);
        ensure(base != MAP_FAILED)
        ensure(madvise(base, file_size, MADV_POPULATE_READ) != -1)
        // Map one more anonymous page to handle attempts to read beyond EOF of stdin gracefully.
        // This would happen either in operator>> while skipping whitespace, or in input(). In the
        // former case, the right thing to do is stop the loop by encountering a non-space
        // character; in the latter case, the right thing to do is to stop the loop by encountering
        // a space character. Something like "\00" works for both cases: it stops (for instance)
        // integer parsing immediately with a zero, and also stops whitespace parsing *soon*.
        ensure(mmap(base + file_size, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != MAP_FAILED)
#   ifdef STDIN_EOF
        // We only really need to do this if we're willing to keep going "after" EOF, not just
        // handle 4k-aligned non-whitespace-terminated input.
        base[file_size + 1] = '0';
#   endif
        ptr = (NonAliasingChar*)base;
    }

    // For people writing cie.tie(0);
    void* tie(nullptr_t) {
        return NULL;
    }

    SIMD void trace_non_whitespace() {
        // We expect long runs here, hence vectorization. Instrinsics break aliasing, and if we
        // interleave ptr modification with SIMD loading, there's going to be an extra memory write
        // on every iteration.
        // This function performs lookahead! This is a bad-bad thing, for a complicated reason. We
        // set the EOF flag upon the access to the guard page, which means that if a SIMD access
        // crosses the boundary between file data and guard page, the flag is set whether we
        // consumed all the lookahead bytes or not. For instance, suppose the data is mapped as
        // follows:
        //     textgoeshere 123|<guard page>
        //             [--------------]
        // Performing the read denoted by [---] will trigger the EOF flag to be set, even though 123
        // is not consumed. We fix this by ensuring lookaheads are aligned, which guarantees that
        // the page boundary is not crossed. This is somewhat beneficial for code size too, because
        // we can use a plain dereference instead of the loadu intrinsic, and perhaps for
        // performance on long strings.
#   ifdef AVX2
        while ((size_t)ptr % 32 && (*ptr < 0 || *ptr > ' ')) {
            ptr++;
        }
        if ((size_t)ptr % 32) {
            return;
        }
        auto p = (__m256i*)ptr;
        __m256i vec, space = _mm256_set1_epi8(' ');
        while (
            vec = _mm256_cmpeq_epi8(space, _mm256_max_epu8(space, *p)),
            _mm256_testz_si256(vec, vec)
        ) {
            p++;
        }
        ptr = (NonAliasingChar*)p + __builtin_ctz(_mm256_movemask_epi8(vec));
#   elif defined(SSE41)
        while ((size_t)ptr % 16 && (*ptr < 0 || *ptr > ' ')) {
            ptr++;
        }
        if ((size_t)ptr % 16) {
            return;
        }
        auto p = (__m128i*)ptr;
        __m128i vec, space = _mm_set1_epi8(' ');
        while (
            vec = _mm_cmpeq_epi8(space, _mm_max_epu8(space, *p)),
            _mm_testz_si128(vec, vec)
        ) {
            p++;
        }
        ptr = (NonAliasingChar*)p + __builtin_ctz(_mm_movemask_epi8(vec));
#   else
        while (*ptr < 0 || *ptr > ' ') {
            ptr++;
        }
#   endif
    }

    SIMD void trace_line() {
        // We expect long runs here, hence vectorization. Instrinsics break aliasing, and if we
        // interleave ptr modification with SIMD loading, there's going to be an extra memory write
        // on every iteration.
        // This function performs lookahead! See the warning in trace_non_whitespace.
#   ifdef AVX2
        while ((size_t)ptr % 32 && *ptr != '\0' && *ptr != '\r' && *ptr != '\n') {
            ptr++;
        }
        if ((size_t)ptr % 32) {
            return;
        }
        auto p = (__m256i*)ptr;
        auto mask = _mm_set_epi64x(0x0000ff0000ff0000, 0x00000000000000ff);
        __m256i vec1, vec2;
        while (
            _mm256_testz_si256(
                vec1 = _mm256_cmpgt_epi8(_mm256_set1_epi8(16), *p),
                // pshufb handles leading 1 in *p as a 0, which is what we want with Unicode
                vec2 = _mm256_shuffle_epi8(_mm256_set_m128i(mask, mask), *p)
            )
        ) {
            p++;
        }
        ptr = (NonAliasingChar*)p + __builtin_ctz(_mm256_movemask_epi8(vec1 & vec2));
#   elif defined(SSE41)
        while ((size_t)ptr % 16 && *ptr != '\0' && *ptr != '\r' && *ptr != '\n') {
            ptr++;
        }
        if ((size_t)ptr % 16) {
            return;
        }
        auto p = (__m128i*)ptr;
        __m128i vec1, vec2;
        while (
            _mm_testz_si128(
                vec1 = _mm_cmpgt_epi8(_mm_set1_epi8(16), *p),
                // pshufb handles leading 1 in *p as a 0, which is what we want with Unicode
                vec2 = _mm_shuffle_epi8(_mm_set_epi64x(0x0000ff0000ff0000, 0x00000000000000ff), *p)
            )
        ) {
            p++;
        }
        ptr = (NonAliasingChar*)p + __builtin_ctz(_mm_movemask_epi8(vec1 & vec2));
#   else
        while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') {
            ptr++;
        }
#   endif
    }

    template<typename T>
    void collect_digits(T& x) {
        while ((*ptr & 0xf0) == 0x30) {
            x = x * 10 + (*ptr++ - '0');
        }
    }

    template<typename T, T = 1>
    void input(T& x) {
        bool negative = is_signed_v<T> && *ptr == '-';
        ptr += negative;
        collect_digits(x = 0);
        if (negative) {
            x = -x;
        }
    }

#   ifdef FLOAT
    template<typename T, typename = decltype(T{1.})>
    void input(T& x) {
        bool negative = *ptr == '-';
        ptr += negative;
        ptr += *ptr == '+';
        auto start = ptr;
        uint64_t n;
        input(n);
        int exponent = 20;  // Offset by 20, for reasons
        if (*ptr == '.') {
            auto after_dot = ++ptr;
            collect_digits(n);
            exponent += after_dot - ptr;
        }
        x = n;
        if (ptr - start >= 19) {
            ptr = start;
            x = 0;
            collect_digits(x);
            if (*ptr == '.') {
                ptr++;
                collect_digits(x);
            }
        }
        if ((*ptr | 0x20) == 'e') {
            ptr++;
            ptr += *ptr == '+';
            int new_exponent;
            input(new_exponent);
            exponent += new_exponent;
        }
        if (0 <= exponent && exponent < 41) {
            // This generates {1e-20, 1e-14, ..., 1e14, 1e20}
            constexpr auto exps = [] {
                array<T, 41> exps{};
                T x = 1;
                for (int i = 21; i--; ) {
                    exps[40 - i] = x;
                    exps[i] = 1 / x;
                    x *= 10;
                }
                return exps;
            }();
            x *= exps[exponent];
        } else {
            while (exponent-- > 20) {
                x *= 10;
            }
            while (++exponent < 20) {
                x *= .1;
            }
        }
        if (negative) {
            x = -x;
        }
    }
#   endif

    void input(bool& x) {
        x = *ptr++ == '1';
    }
    void input(char& x) {
        x = *ptr++;
    }

#   ifdef CHAR_WITH_SIGN_IS_GLYPH
    void input(uint8_t& x) {
        x = *ptr++;
    }
    void input(int8_t& x) {
        x = *ptr++;
    }
#   endif

    void input(string& value) {
        auto start = ptr;
        trace_non_whitespace();
        // We know there's no overlap, so avoid doing this for a little bit of performance:
        // value.assign((const char*)start, ptr - start);
        ((basic_string<UninitChar>&)value).resize(ptr - start);
        memcpy(value.data(), start, ptr - start);
    }

#   ifdef COMPLEX
    template<typename T>
    void input(complex<T>& value) {
        T real_part, imag_part{};
        if (*ptr == '(') {
            ptr++;
            input(real_part);
            if (*ptr++ == ',') {
                *this >> imag_part;
                ptr++;
            }
        } else {
            input(real_part);
        }
        value = {real_part, imag_part};
    }
#   endif

#   ifdef BITSET
    template<size_t N>
    SIMD void input(bitset<N>& value) {
#   ifdef STDIN_EOF
        // As we always read N bytes, we might read past the end of the file in case EOF happens.
        // Luckily, we are allowed to overread up to 4095 bytes after EOF (because there's a
        // 4096-page and its second byte is non-whitespace). Therefore, we only have to check for
        // EOF for large enough N, and in this case the overhead is small enough.
        if (N >= 4096 && !*this) {
            return;
        }
#   endif
        auto i = N;
#   ifdef AVX2
        while (i % 32) {
            value[--i] = *ptr++ == '1';
        }
        auto p = (__m256i*)ptr;
        i /= 32;
        while (i) {
            long a = 0x0001020304050607;
            ((uint32_t*)&value)[--i] = __bswap_32(_mm256_movemask_epi8(_mm256_shuffle_epi8(_mm256_loadu_si256(p++) << 7, _mm256_set_epi64x(a + ONE_BYTES * 24, a + ONE_BYTES * 16, a + ONE_BYTES * 8, a))));
        }
        ptr = (NonAliasingChar*)p;
#   elif defined(SSE41)
        while (i % 16) {
            value[--i] = *ptr++ == '1';
        }
        auto p = (__m128i*)ptr;
        i /= 16;
        while (i) {
            long a = 0x0001020304050607;
            ((uint16_t*)&value)[--i] = _mm_movemask_epi8(_mm_shuffle_epi8(_mm_loadu_si128(p++) << 7, _mm_set_epi64x(a, a + ONE_BYTES * 8)));
        }
        ptr = (NonAliasingChar*)p;
#   else
        while (i % 8) {
            value[--i] = *ptr++ == '1';
        }
        auto p = (long*)ptr;
        i /= 8;
        while (i) {
            ((char*)&value)[--i] = ((*p++ & ONE_BYTES) * BITSET_SHIFT) >> 56;
        }
        ptr = (NonAliasingChar*)p;
#   endif
    }
#   endif

    void input(line_t& line) {
#   ifdef STDIN_EOF
        if (*ptr) {
#   endif
            auto start = ptr;
            trace_line();
            // We know there's no overlap, so avoid doing this for a little bit of performance:
            // value.value.assign((const char*)start, ptr - start);
            ((basic_string<UninitChar>&)line.value).resize(ptr - start);
            memcpy(line.value.data(), (char*)start, ptr - start);
            ptr += *ptr == '\r';
#   ifdef STDIN_EOF
        } else {
            // If we're on the null byte, it's EOF and we should signal that by putting ptr past the
            // start of the guard page.
            ptr = (NonAliasingChar*)base + file_size;
        }
#   endif
        ptr++;
    }

    template<typename T>
    blazingio_istream& operator>>(T& value) {
        if (!is_same_v<T, line_t>) {
            // Skip whitespace. 0..' ' are not all whitespace, but we only care about well-formed input.
            // We expect short runs here, hence no vectorization.
            while (0 <= *ptr && *ptr <= ' ') {
                ptr++;
            }
        }

        input(value);
        return *this;
    }

#   ifdef STDIN_EOF
    operator bool() {
        return !!*this;
    }
    bool operator!() {
        return (char*)ptr > base + file_size;
    }
#   endif
};

struct blazingio_ostream {
    char* base;
    NonAliasingChar* ptr;

#   ifdef LUT
    inline static char decimal_lut[200];
#   endif

    blazingio_ostream() {
        // Avoid MAP_SHARED: it turns out it's pretty damn inefficient compared to a write at the
        // end. This also allows us to allocate memory immediately without waiting for freopen,
        // because we'll only use the fd in the destructor.
        base = (char*)mmap(NULL, BIG, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        ensure(base != MAP_FAILED)
        ptr = (NonAliasingChar*)base;

#   ifdef LUT
        // The code gets shorter if we initialize LUT here as opposed to during compile time.
        for (int i = 0; i < 100; i++) {
            decimal_lut[i * 2] = '0' + i / 10;
            decimal_lut[i * 2 + 1] = '0' + i % 10;
        }
#   endif
    }
    ~blazingio_ostream() {
#   ifdef SPLICE
        ssize_t n_written = 1;
        while (n_written > 0) {
            iovec iov{base, (size_t)ptr - (size_t)base};
            base += (n_written = vmsplice(STDOUT_FILENO, &iov, 1, SPLICE_F_GIFT));
        }
        // Perhaps not a pipe?
        if (n_written) {
            base++;
            do {
                base += (n_written = write(STDOUT_FILENO, base, (char*)ptr - base));
            } while (n_written > 0);
            ensure(n_written != -1)
        }
#   else
        ssize_t n_written = 1;
        while (n_written > 0) {
            base += (n_written = write(STDOUT_FILENO, base, (char*)ptr - base));
        }
        ensure(n_written != -1)
#   endif
    }

    void print(char value) {
        *ptr++ = value;
    }
#   ifdef CHAR_WITH_SIGN_IS_GLYPH
    void print(uint8_t value) {
        *ptr++ = value;
    }
    void print(int8_t value) {
        *ptr++ = value;
    }
#   endif
    void print(bool value) {
        *ptr++ = '0' + value;
    }

    template<typename T, int MinDigits, int MaxDigits, T Factor = 1>
    void write_int_split(T value, T interval) {
        if constexpr (MaxDigits == 1) {
            if (MinDigits >= 1 || value >= Factor) {
                *ptr++ = '0' + interval;
            }
#   ifdef LUT
        } else if constexpr (MaxDigits == 2) {
            if (MinDigits >= 2 || value >= 10 * Factor) {
                print(decimal_lut[interval * 2]);
            }
            if (MinDigits >= 1 || value >= Factor) {
                print(decimal_lut[interval * 2 + 1]);
            }
#   endif
        } else {
            constexpr auto computed = [] {
                int low_digits = 1;
                T coeff = 10;
                while ((low_digits *= 2) < MaxDigits) {
                    coeff *= coeff;
                }
                return pair{low_digits / 2, coeff};
            }();
            constexpr int low_digits = computed.first;
            constexpr T coeff = computed.second;
            write_int_split<T, max(0, MinDigits - low_digits), MaxDigits - low_digits, T(Factor * coeff)>(value, interval / coeff);
            write_int_split<T, min(MinDigits, low_digits), low_digits, Factor>(value, interval % coeff);
        }
    }

    template<typename T, T = 1>
    void print(T value) {
        make_unsigned_t<T> abs = value;
        if (value < 0) {
            print('-');
            abs = -abs;
        }
        write_int_split<
            decltype(abs),
            1,
            array{3, 5, 10, 20}[__builtin_ctz(sizeof(value))]
        >(abs, abs);
    }

#   ifdef FLOAT
    template<typename T, typename = decltype(T{1.})>
    void print(T value) {
        if (value < 0) {
            print('-');
            value = -value;
        }
        // At least it isn't \write18...
        auto write12 = [&] {
            write_int_split<uint64_t, 12, 12>(value * 1e12, value * 1e12);
        };
        if (!value) {
            return print('0');
        }
        if (value >= 1e16) {
            value *= 1e-16;
            int exponent = 16;
            while (value >= 1) {
                value *= .1;
                exponent++;
            }
            print("0.");
            write12();
            print('e');
            print(exponent);
        } else if (value >= 1) {
            uint64_t whole = value;
            print(whole);
            if (value -= whole) {
                print('.');
                write12();
            }
        } else {
            print("0.");
            write12();
        }
    }
#   endif

    void print(const char* value) {
        // We'd prefer strcpy without null terminator here, but perhaps strcpy itself suffices. It's
        // also a builtin in GCC, which means outputting a constant string is going to be optimized
        // into a mov or two!
        ptr = (NonAliasingChar*)stpcpy((char*)ptr, value);
    }
#   ifdef CHAR_WITH_SIGN_IS_GLYPH
    void print(const uint8_t* value) {
        print((char*)value);
    }
    void print(const int8_t* value) {
        print((char*)value);
    }
#   endif

    // std::string is inferred from this:
    void print(string_view value) {
        memcpy(ptr, value.data(), value.size());
        ptr += value.size();
    }

#   ifdef COMPLEX
    template<typename T>
    void print(complex<T> value) {
        *this << '(' << value.real() << ',' << value.imag() << ')';
    }
#   endif

#   ifdef BITSET
    template<size_t N>
    SIMD void print(const bitset<N>& value) {
        auto i = N;
#   ifdef AVX2
        while (i % 32) {
            *ptr++ = '0' + value[--i];
        }
        auto p = (__m256i*)ptr;
        i /= 32;
        auto b = _mm256_set1_epi64x(0x0102040810204080);
        while (i) {
            _mm256_storeu_si256(
                p++,
                _mm256_sub_epi8(
                    _mm256_set1_epi8('0'),
                    _mm256_cmpeq_epi8(
                        _mm256_shuffle_epi8(
                            _mm256_set1_epi32(((uint32_t*)&value)[--i]),
                            _mm256_set_epi64x(0, ONE_BYTES, ONE_BYTES * 2, ONE_BYTES * 3)
                        ) & b,
                        b
                    )
                )
            );
        }
        ptr = (NonAliasingChar*)p;
#   elif defined(SSE41)
        while (i % 16) {
            *ptr++ = '0' + value[--i];
        }
        auto p = (__m128i*)ptr;
        i /= 16;
        auto b = _mm_set1_epi64x(0x0102040810204080);
        while (i) {
            _mm_storeu_si128(
                p++,
                _mm_sub_epi8(
                    _mm_set1_epi8('0'),
                    _mm_cmpeq_epi8(
                        _mm_shuffle_epi8(
                            _mm_set1_epi16(((uint16_t*)&value)[--i]),
                            _mm_set_epi64x(0, ONE_BYTES)
                        ) & b,
                        b
                    )
                )
            );
        }
        ptr = (NonAliasingChar*)p;
#   else
        while (i % 8) {
            *ptr++ = '0' + value[--i];
        }
        auto p = (long*)ptr;
        i /= 8;
        while (i) {
            *p++ = ((BITSET_SHIFT * ((uint8_t*)&value)[--i]) >> 7) & ONE_BYTES | (ONE_BYTES * 0x30);
        }
        ptr = (NonAliasingChar*)p;
#   endif
    }
#   endif

    template<typename T>
    blazingio_ostream& operator<<(const T& value) {
        print(value);
        return *this;
    }

    blazingio_ostream& operator<<(blazingio_ostream& (*func)(blazingio_ostream&)) {
        return func(*this);
    }
};

#   ifdef CERR
struct blazingio_ignoreostream {
    template<typename T>
    blazingio_ignoreostream& operator<<(const T& value) {
        return *this;
    }
    blazingio_ignoreostream& operator<<(blazingio_ignoreostream& (*func)(blazingio_ignoreostream&)) {
        return func(*this);
    }
};
#   endif

}

namespace std {
    blazingio::blazingio_istream blazingio_cin;
    blazingio::blazingio_ostream blazingio_cout;
#   ifdef CERR
    blazingio::blazingio_ignoreostream blazingio_cerr;
#   endif

    blazingio::blazingio_istream& getline(blazingio::blazingio_istream& stream, string& value) {
        blazingio::line_t line{value};
        return stream >> line;
    }

    blazingio::blazingio_ostream& endl(blazingio::blazingio_ostream& stream) {
        return stream << '\n';
    }
    blazingio::blazingio_ostream& flush(blazingio::blazingio_ostream& stream) {
        return stream;
    }

#   ifdef CERR
    blazingio::blazingio_ignoreostream& endl(blazingio::blazingio_ignoreostream& stream) {
        return stream;
    }
    blazingio::blazingio_ignoreostream& flush(blazingio::blazingio_ignoreostream& stream) {
        return stream;
    }
#   endif
}

#define cin blazingio_cin
#define cout blazingio_cout

#   ifdef CERR
#ifdef ONLINE_JUDGE
#define cerr blazingio_cerr
#define clog blazingio_cerr
#endif
#   endif

#   endif
