#include <array>
!ifdef BITSET
#include <bitset>
!endif
!ifdef COMPLEX
#include <complex>
!endif
#include <cstring>
!ifdef SPLICE
#include <fcntl.h>
!endif
@include
@case *-x86_64+avx2,*-x86_64+sse4.1 <immintrin.h>
@case *-aarch64+neon <arm_neon.h>
@case *-x86_64+none,*-aarch64+none none
@end
@ondemand windows-*
#include <stdint.h>
@end
@include
@case linux-*,macos-* <sys/mman.h>
@case windows-* <windows.h>
@end
#include <unistd.h>

@define #SIMD
@case *-x86_64+avx2 __attribute__((target("avx2")))
@case *-x86_64+sse4.1 __attribute__((target("sse4.1")))
@case *-x86_64+none,*-aarch64
@end

@define SIMD_SIZE
@case *-x86_64+avx2 32
@case *-x86_64+sse4.1,*-aarch64+neon 16
@case *-x86_64+none,*-aarch64+none 8
@end

@define #SIMD_TYPE
@case *-x86_64+avx2 __m256i
@case *-x86_64+sse4.1 __m128i
@case *-aarch64+neon uint8x16_t
@case *-x86_64+none,*-aarch64+none uint64_t
@end

// This is ridiculous but necessary for clang codegen to be at least somewhat reasonable --
// otherwise it resorts to way too many memory accesses.
#define INLINE __attribute__((always_inline))

!ifdef INTERACTIVE
#define FETCH fetch(),
!else
!define FETCH
!endif

#define ensure(x) if (!(x)) abort();

@match
@case linux-*,macos-*
@case windows-*
LONG vectored_exception_handler(_EXCEPTION_POINTERS*);
@end

namespace blazingio {

using namespace std;

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

!ifdef BITSET
const uint64_t ONE_BYTES = -1ULL / 255
@ondemand *-x86_64+none,*-aarch64+none
, BITSET_SHIFT = 0x8040201008040201
@end
;
!endif

// Actually 0x0102040810204080
!define POWERS_OF_TWO -3ULL / 254

struct line_t {
    string& value;
};

!ifndef INTERACTIVE
!define istream_impl blazingio_istream
!endif

!ifdef INTERACTIVE
// Allocate one more byte for null terminator as used by parsing routines. We might want to
// lookahead over this byte though, so add 32 instead of 1.
static NonAliasingChar buffer[65568];

template<bool Interactive>
!endif
struct istream_impl {
    NonAliasingChar* end;
    NonAliasingChar* ptr;

!ifdef INTERACTIVE
    void init_assume_file(off_t file_size) {
!else
    blazingio_istream() {
        off_t file_size = lseek(STDIN_FILENO, 0, SEEK_END);
        ensure(~file_size)
!endif
        // Round to page size.
        (file_size += 4095) &= -4096;
@match
@case windows-*
        // Windows is a mess. With allocation granularity 64k and page size 4k we don't always have
        // the option of mapping a zero page immediately after contents. For instance, mapping a 32k
        // file will leave the second half of the 64k section unmapped, triggering a page fault upon
        // access without letting us to map the page right. Therefore, we map the file and one more
        // page; this both gives us free space to work with and guarantees at most 64k bytes trap.
        // Aligning the sizes to 64k, we then remap the last 64k with rw memory and read it from
        // file. This is a mix mmap-based file handling with read-based file handling and is
        // hopefully more efficient than a pure read-based method.
        // Find free space
        char* base = (char*)VirtualAlloc(NULL, file_size + 4096, MEM_RESERVE, PAGE_NOACCESS);
        ensure(base)
        ensure(VirtualFree(base, 0, MEM_RELEASE))
        // Map the file there
        size_t mmaped_region_size = (file_size + 4096) & -65536;
        // If we remove this if and always call CreateFileMapping, it's going to interpret 0 as
        // "max", which we don't want.
        if (mmaped_region_size)
            ensure(
                MapViewOfFileEx(
                    CreateFileMapping(
                        GetStdHandle(STD_INPUT_HANDLE),
                        NULL,
                        PAGE_READONLY,
                        // XXX: This assumes the file fits in ~4 GB by putting the size in the low
                        // DWORD only
                        0,
                        mmaped_region_size,
                        NULL
                    ),
                    FILE_MAP_READ,
                    0,
                    0,
                    0,
                    base
                ) == base
            )
        // Read into the start of a 64k region
        ensure(
            VirtualAlloc(base + mmaped_region_size, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
            == base + mmaped_region_size
        )
        ensure(~_lseek(STDIN_FILENO, mmaped_region_size, SEEK_SET))
        _read(STDIN_FILENO, base + mmaped_region_size, 65536);
        // WIN32_MEMORY_RANGE_ENTRY range{end - file_size, (size_t)file_size};
        // ensure(PrefetchVirtualMemory(GetCurrentProcess(), 1, &range, 0) != -1)
@case linux-*,macos-*
        char* base = (char*)mmap(NULL, file_size + 4096, PROT_READ, MAP_PRIVATE, STDIN_FILENO, 0);
        ensure(base != MAP_FAILED)
        // Map one more anonymous page to handle attempts to read beyond EOF of stdin gracefully.
        // This would happen either in operator>> while skipping whitespace, or in input(). In the
        // former case, the right thing to do is stop the loop by encountering a non-space
        // character; in the latter case, the right thing to do is to stop the loop by encountering
        // a space character. Something like "\00" works for both cases: it stops (for instance)
        // integer parsing immediately with a zero, and also stops whitespace parsing *soon*.
        ensure(mmap(base + file_size, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != MAP_FAILED)
@end
        end = (NonAliasingChar*)base + file_size;
!ifdef STDIN_EOF
        // We only really need to do this if we're willing to keep going "after" EOF, not just
        // handle 4k-aligned non-whitespace-terminated input.
        end[1] = '0';
!endif
        ptr = (NonAliasingChar*)base;
    }

!ifdef INTERACTIVE
    void init_assume_interactive() {
        end = ptr = buffer;
    }
!endif

!ifndef INTERACTIVE
    // For people writing cie.tie(0);
    void* tie(nullptr_t) {
        return NULL;
    }
!endif

!ifdef INTERACTIVE
    INLINE void fetch() {
        if (Interactive && __builtin_expect(ptr == end, 0)) {
!ifdef HOIST_GLOBALS_ON_INTERACTIVE_INPUT
            // There's a bit of ridiculous code with questionable choices below. What we *want* is:
            //     off_t n_read = read(STDIN_FILENO, buffer, 65536);
            // Unfortunately, read() is an external call, which means it can override globals. Even
            // though blazingio_cin is static, there's no guarantee read() doesn't map to a symbol
            // from the same translation unit, so GCC assumes read() may read or modify ptr or end.
            // This causes it to spill the values to memory in the hot loop, which is a bad-bad
            // thing. Therefore we have to avoid the call to read() and roll our own inline
            // assembly. The *obvious* way to write that is
            //     off_t n_read = SYS_read;
            //     asm volatile(
            //         "syscall"
            //         : "+a"(n_read)
            //         : "D"(STDIN_FILENO), "S"(buffer), "d"(65536)
            //         : "rcx", "r11", "memory"
            //     );
            // ...but that suffers from the same consequences as the call to read() because of the
            // memory clobber. The second-most obvious way to rewrite this is documented by GCC as
            //     off_t n_read = SYS_read;
            //     asm volatile(
            //         "syscall"
            //         : "+a"(n_read), "+m"(buffer)
            //         : "D"(STDIN_FILENO), "S"(buffer), "d"(65536)
            //         : "rcx", "r11"
            //     );
            // ...which should theoretically be optimal because 'buffer' is of type
            // NonAliasingChar[], meaning it can't alias with ptr and other fields. Unfortunately,
            // that doesn't work *either* because of a missed optimization that hasn't been fixed
            // for years (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63900), so we sort of
            // have to resort to UB and compiler-specific optimizations here. We thus remove any
            // mention of clobbering anything NonAliasingChar-related and simulate the behavior "as
            // if" the buffer was *reallocated* on each read by telling GCC that the syscall might
            // change 'ptr' (it actually won't, but GCC won't be able to misoptimize based on this):
            //     ptr = buffer;
            //     off_t n_read = SYS_read;
            //     asm volatile(
            //         "syscall"
            //         : "+a"(n_read), "+S"(ptr)
            //         : "D"(STDIN_FILENO), "d"(65536)
            //         : "rcx", "r11"
            //     );
            // UNFORTUNATELY, this doesn't work *either* due to *yet another* missed optimization:
            // even though ptr is clearly loaded into a register, GCC assumes memory might still be
            // modified, so we have to load ptr into a local variable and then put it back, like
            // this:
@define !SYSCALL_NO_REGISTER
@case linux-* "x8"
@case macos-* "x16"
@end
@define !SVC
@case linux-* ""
@case macos-* "x80"
@end
@match
@case *-x86_64
            off_t n_read = SYS_read;
            NonAliasingChar* rsi = buffer;
            asm volatile(
                // Put a 0 byte after data for convenience of parsing routines. For some reason,
                // doing this in an asm statement results in better codegen.
                // XXX: Handling errors here is complicated, because on Linux syscall will return
                // a small negative number in rax, leading to OOB write, while on XNU the syscall
                // will return a small positive number in rax and set a carry flag we ignore, making
                // it seem like we've just read a few bytes. Neither case is handled correctly, and
                // 'ensure(n_read >= 0)' just hides the error, so let us explicitly state we don't
                // support errors returned from read(2) for now. This should probably be fixed
                // later.
                "syscall; movb $0, (%%rsi,%%rax);"
                : "+a"(n_read), "+S"(rsi)
                : "D"(STDIN_FILENO), "d"(65536)
                : "rcx", "r11"
            );
            ptr = rsi;
@case *-aarch64 wrap
            // Linux:  svc 0, syscall number in x8
            // Mac OS: svc 0x80, syscall number in x16
            register long
                n_read asm("x0") = STDIN_FILENO,
                arg1 asm("x1") = (long)buffer,
                arg2 asm("x2") = 65536,
                syscall_no asm(SYSCALL_NO_REGISTER) = SYS_read;
            asm volatile(
                "svc 0" SVC "; strb wzr, [x1, x0]"
                : "+r"(n_read), "+r"(arg1)
                : "r"(syscall_no), "r"(arg2)
            );
            ptr = (NonAliasingChar*)arg1;
@end
!else
            off_t n_read = read(STDIN_FILENO, ptr = buffer, 65536);
            ensure(~n_read)
!endif
            end = ptr + n_read;
!ifdef STDIN_EOF
            if (!n_read) {
                // This is an attempt to read past EOF. Simulate this just like with files, with
                // "\00". Be careful to use 'buffer' instead of 'ptr' here -- using the latter
                // confuses GCC's optimizer for some reason.
                buffer[0] = 0;
                buffer[1] = '0';
                // We want ptr == end to evaluate to false.
                end = NULL;
            }
!endif
        }
    }
!endif

    template<typename T>
    INLINE void collect_digits(T& x) {
        while (FETCH (*ptr & 0xf0) == 0x30)
            x = x * 10 + (*ptr++ - '0');
    }

    template<typename T, T = 1>
    INLINE void input(T& x) {
        bool negative = is_signed_v<T> && (FETCH *ptr == '-');
        ptr += negative;
        collect_digits(x = 0);
        x = negative ? -x : x;
    }

!ifdef FLOAT
    template<typename T, typename = decltype(T{1.})>
    INLINE void input(T& x) {
        bool negative = (FETCH *ptr == '-');
        ptr += negative;
        FETCH ptr += *ptr == '+';

        uint64_t n = 0;
        int i = 0;
        for (; i < 18 && (FETCH *ptr & 0xf0) == 0x30; i++)
            n = n * 10 + *ptr++ - '0';
        int exponent = 20;  // Offset by 20, for reasons
        bool has_dot = *ptr == '.';
        ptr += has_dot;
        for (; i < 18 && (FETCH *ptr & 0xf0) == 0x30; i++) {
            n = n * 10 + *ptr++ - '0';
            exponent -= has_dot;
        }
        x = n;
        while ((FETCH *ptr & 0xf0) == 0x30) {
            x = n * 10 + *ptr++ - '0';
            exponent -= has_dot;
        }
        if (*ptr == '.') {
            ptr++;
            has_dot = true;
        }
        while ((FETCH *ptr & 0xf0) == 0x30) {
            x = n * 10 + *ptr++ - '0';
            exponent -= has_dot;
        }

        if ((*ptr | 0x20) == 'e') {
            ptr++;
            FETCH ptr += *ptr == '+';
            int new_exponent;
            input(new_exponent);
            exponent += new_exponent;
        }
        if (0 <= exponent && exponent < 41) {
            // This generates {1e-20, 1e-14, ..., 1e14, 1e20}
            static constexpr auto exps = [] {
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
            while (exponent-- > 20)
                x *= 10;
            while (++exponent < 20)
                x *= .1;
        }
        x = negative ? -x : x;
    }
!endif

    INLINE void input(bool& x) {
        FETCH x = *ptr++ == '1';
    }
    INLINE void input(char& x) {
        FETCH x = *ptr++;
    }

!ifdef CHAR_WITH_SIGN_IS_GLYPH
    INLINE void input(uint8_t& x) {
        FETCH x = *ptr++;
    }
    INLINE void input(int8_t& x) {
        FETCH x = *ptr++;
    }
!endif

    SIMD void input_string_like(string& value, NonAliasingChar* (*trace)(NonAliasingChar*)) {
        auto start = ptr;
        ptr = trace(ptr);

        // We know that [start; ptr) does not overlap 'value'. std::string::assign doesn't
        // know that and will perform a runtime check to determine if it need to handle
        // aliasing strings gracefully. This takes a bit of time, so we *used to* do the
        // following instead:
        //     struct UninitChar { UninitChar& operator=(UninitChar) { return *this; } };
        //     ((basic_string<UninitChar>&)value).resize(ptr - start);
        //     memcpy(value.data(), start, ptr - start);
        // This worked just fine, but libc++ forbids this code because UninitChar is not
        // a trivial type. Therefore, disable this optimization.
        value.assign((const char*)start, ptr - start);

!ifdef INTERACTIVE
        while (Interactive && ptr == end) {
            // We have read *some* data, but stumbled upon an unfetched chunk and thus have to load
            // more. We can't reuse the same code as we want to append to the string instead of
            // replacing it. fetch() will set 'end = NULL' on EOF here, even though the string/line
            // exists and we don't want to report end at the moment; therefore, patch 'end'.
            if (FETCH !end) {
                end = ptr;
                break;
            }
            // Abuse the fact that ptr points at buffer after a non-trivial fetch to avoid storing
            // start.
            ptr = trace(ptr);
            value.append(buffer, ptr);
        }
!endif
    }

    SIMD void input(string& value) {
        input_string_like(value, [](NonAliasingChar* ptr) SIMD {
            // We expect long runs here, hence vectorization. Instrinsics break aliasing, and if we
            // interleave ptr modification with SIMD loading, there's going to be an extra memory
            // write on every iteration.
@match
@case *-x86_64+avx2 wrap
            auto p = (__m256i*)ptr;
            __m256i vec, space = _mm256_set1_epi8(' ');
            while (
                vec = _mm256_cmpeq_epi8(space, _mm256_max_epu8(space, _mm256_loadu_si256(p))),
                _mm256_testz_si256(vec, vec)
            )
                p++;
            return (NonAliasingChar*)p + __builtin_ctz(_mm256_movemask_epi8(vec));
@case *-x86_64+sse4.1 wrap
            auto p = (__m128i*)ptr;
            __m128i vec, space = _mm_set1_epi8(' ');
            while (
                vec = _mm_cmpeq_epi8(space, _mm_max_epu8(space, _mm_loadu_si128(p))),
                _mm_testz_si128(vec, vec)
            )
                p++;
            return (NonAliasingChar*)p + __builtin_ctz(_mm_movemask_epi8(vec));
@case *-aarch64+neon wrap
            auto p = (uint8x16_t*)ptr;
            uint64x2_t vec;
            while (vec = (uint64x2_t)(*p <= ' '), !(vec[0] | vec[1]))
                p++;
            return (NonAliasingChar*)p + (vec[0] ? 0 : 8) + __builtin_ctzll(vec[0] ?: vec[1]) / 8;
@case *-x86_64+none,*-aarch64+none
            while (*ptr < 0 || *ptr > ' ')
                ptr++;
            return ptr;
@end
        });
    }

    SIMD void input(line_t& line) {
!ifdef STDIN_EOF
        // Detect if we're at the end of the file. getline has to be handled differently from other
        // inputs because it treats a null byte as a line terminator and would thus wrongly assume
        // there's an empty line after EOF.
        if (FETCH !*ptr) {
            // Trigger EOF condition.
            end = NULL;
            return;
        }
!endif

        input_string_like(line.value, [](NonAliasingChar* ptr) SIMD {
            // We expect long runs here, hence vectorization. Instrinsics break aliasing, and if we
            // interleave ptr modification with SIMD loading, there's going to be an extra memory
            // write on every iteration.
@match
@case *-x86_64+avx2 wrap
            auto p = (__m256i*)ptr;
            auto mask = _mm_set_epi64x(0x0000ff0000ff0000, 0x00000000000000ff);
            __m256i vec, vec1, vec2;
            while (
                vec = _mm256_loadu_si256(p),
                _mm256_testz_si256(
                    vec1 = _mm256_cmpgt_epi8(_mm256_set1_epi8(16), vec),
                    // pshufb handles leading 1 in vec as a 0, which is what we want with Unicode
                    vec2 = _mm256_shuffle_epi8(_mm256_set_m128i(mask, mask), vec)
                )
            )
                p++;
            return (NonAliasingChar*)p + __builtin_ctz(_mm256_movemask_epi8(vec1 & vec2));
@case *-x86_64+sse4.1 wrap
            auto p = (__m128i*)ptr;
            __m128i vec, vec1, vec2;
            while (
                vec = _mm_loadu_si128(p),
                _mm_testz_si128(
                    vec1 = _mm_cmpgt_epi8(_mm_set1_epi8(16), vec),
                    // pshufb handles leading 1 in vec as a 0, which is what we want with Unicode
                    vec2 = _mm_shuffle_epi8(
                        _mm_set_epi64x(0x0000ff0000ff0000, 0x00000000000000ff),
                        vec
                    )
                )
            )
                p++;
            return (NonAliasingChar*)p + __builtin_ctz(_mm_movemask_epi8(vec1 & vec2));
@case *-aarch64+neon wrap
            auto p = (uint8x16_t*)ptr;
            uint64_t table[] = {0x00000000000000ff, 0x0000ff0000ff0000};
            uint64x2_t vec;
            while (vec = (uint64x2_t)vqtbl1q_u8(*(uint8x16_t*)table, *p), !(vec[0] | vec[1]))
                p++;
            return (NonAliasingChar*)p + (vec[0] ? 0 : 8) + __builtin_ctzll(vec[0] ?: vec[1]) / 8;
@case *-x86_64+none,*-aarch64+none
            while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n')
                ptr++;
            return ptr;
@end
        });

        // Skip \n and \r\n
        ptr += *ptr == '\r';
        ptr += *ptr == '\n';
    }

!ifdef COMPLEX
    template<typename T>
    INLINE void input(complex<T>& value) {
        T real_part, imag_part{};
        if (FETCH *ptr == '(') {
            ptr++;
            input(real_part);
            if (FETCH *ptr++ == ',') {
!ifdef INTERACTIVE
                rshift_impl(imag_part);
!else
                *this >> imag_part;
!endif
                ptr++;
            }
        } else
            input(real_part);
        value = {real_part, imag_part};
    }
!endif

!ifdef BITSET
    template<size_t N>
    SIMD void input(bitset<N>& value) {
!ifdef STDIN_EOF
        // As we always read N bytes, we might read past the end of the file in case EOF happens.
        // Luckily, we are allowed to overread up to 4095 bytes after EOF (because there's a
        // 4096-page and its second byte is non-whitespace). Therefore, we only have to check for
        // EOF for large enough N, and in this case the overhead is small enough.
        if (N >= 4096 && !*this)
            return;
!endif
        ssize_t i = N;
!ifdef INTERACTIVE
        while (i) {
            if (FETCH i % SIMD_SIZE || end - ptr < SIMD_SIZE)
                value[--i] = *ptr++ == '1';
            else {
!else
        while (i % SIMD_SIZE)
            value[--i] = *ptr++ == '1';
!endif
                auto p = (SIMD_TYPE*)ptr;
!ifdef INTERACTIVE
                for (size_t j = 0; j < min(i, end - ptr) / SIMD_SIZE; j++) {
!else
                while (i) {
!endif
                    i -= SIMD_SIZE;
@match
@case *-x86_64+avx2
                    // This is actually 0x0001020304050607
                    uint64_t a = -1ULL / 65025;
                    ((uint32_t*)&value)[i / 32] = __builtin_bswap32(
                        _mm256_movemask_epi8(
                            _mm256_shuffle_epi8(
                                _mm256_loadu_si256(p++) << 7,
                                _mm256_set_epi64x(
                                    a + ONE_BYTES * 24,
                                    a + ONE_BYTES * 16,
                                    a + ONE_BYTES * 8,
                                    a
                                )
                            )
                        )
                    );
@case *-x86_64+sse4.1
                    // This is actually 0x0001020304050607
                    uint64_t a = -1ULL / 65025;
                    ((uint16_t*)&value)[i / 16] = _mm_movemask_epi8(
                        _mm_shuffle_epi8(
                            _mm_loadu_si128(p++) << 7,
                            _mm_set_epi64x(a, a + ONE_BYTES * 8)
                        )
                    );
@case *-aarch64+neon wrap
                    auto masked = (uint8x16_t)vdupq_n_u64(POWERS_OF_TWO) & ('0' - *p++);
                    auto zipped = vzip_u8(vget_high_u8(masked), vget_low_u8(masked));
                    ((uint16_t*)&value)[i / 16] = vaddvq_u16(
                        (uint16x8_t)vcombine_u8(zipped.val[0], zipped.val[1])
                    );
@case *-x86_64+none,*-aarch64+none
                    ((char*)&value)[i / 8] = ((*p++ & ONE_BYTES) * BITSET_SHIFT) >> 56;
@end
                }
                ptr = (NonAliasingChar*)p;
!ifdef INTERACTIVE
            }
        }
!endif
    }
!endif

    template<typename T>
!ifdef INTERACTIVE
    INLINE void rshift_impl(T& value) {
!else
    INLINE blazingio_istream& operator>>(T& value) {
!endif
        if (!is_same_v<T, line_t>)
            // Skip whitespace. 0..' ' are not all whitespace, but we only care about well-formed input.
            // We expect short runs here, hence no vectorization.
            while (FETCH 0 <= *ptr && *ptr <= ' ')
                ptr++;

        input(value);
!ifndef INTERACTIVE
        return *this;
!endif
    }

!ifdef STDIN_EOF
    operator bool() {
        return !!*this;
    }
    bool operator!() {
        return ptr > end;
    }
!endif
};

!ifdef INTERACTIVE
struct blazingio_istream {
    off_t file_size;
    istream_impl<false> file;
    istream_impl<true> interactive;

    blazingio_istream() {
        file_size = lseek(STDIN_FILENO, 0, SEEK_END);
        // We want to switch to a pipe-based method if the file is a special device. On Linux, this
        // can be detected by lseek returning -1. Mac OS, however, returns 0 from lseek(SEEK_END) on
        // some special files, e.g. /dev/null, /dev/zero, and ptys. If we compared the return value
        // just to -1, the check would pass and mmap'ing the file would fail, crashing the program.
        // Therefore, use > 0 instead, even though it's a bit longer.
        file_size > 0
            ? file.init_assume_file(file_size)
            : interactive.init_assume_interactive();
    }

    // For people writing cie.tie(0);
    void* tie(nullptr_t) {
        return NULL;
    }

    template<typename T>
    INLINE blazingio_istream& operator>>(T& value) {
        __builtin_expect(file_size > 0, 1)
            ? file.rshift_impl(value)
            : interactive.rshift_impl(value);
        return *this;
    }

!ifdef STDIN_EOF
    operator bool() {
        return !!*this;
    }
    bool operator!() {
        return __builtin_expect(file_size > 0, 1) ? !file : !interactive;
    }
!endif
};
!endif

struct blazingio_ostream {
    char* base;
    NonAliasingChar* ptr;

!ifdef LUT
    inline static char decimal_lut[200];
!endif

    blazingio_ostream() {
        // We *could* use 'base = new char[0x40000000];' instead of mmap-based allocation here, but
        // that would lead to problems on systems without overcommit, such as Windows.
@match
@case linux-*,macos-*
        // Avoid MAP_SHARED: it turns out it's pretty damn inefficient compared to a write at the
        // end. This also allows us to allocate memory immediately without waiting for freopen,
        // because we'll only use the fd in the destructor.
        base = (char*)mmap(
            NULL,
            0x40000000,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
            -1,
            0
        );
        ensure(base != MAP_FAILED)
@case windows-*
        // Windows doesn't support anything like MAP_NORESERVE or overcommit. Therefore, reserve
        // memory and use guard pages to extend the committed region.
        ensure(base = (char*)VirtualAlloc(NULL, 0x40000000, MEM_RESERVE, PAGE_READWRITE))
        ensure(VirtualAlloc(base, 4096, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD))
        AddVectoredExceptionHandler(true, vectored_exception_handler);
        ptr = (NonAliasingChar*)base;
@end
!ifdef LUT
        // The code gets shorter if we initialize LUT here as opposed to during compile time.
        for (int i = 0; i < 100; i++) {
            decimal_lut[i * 2] = '0' + i / 10;
            decimal_lut[i * 2 + 1] = '0' + i % 10;
        }
!endif
    }
    ~blazingio_ostream() {
!ifdef INTERACTIVE
        do_flush();
    }

    void do_flush() {
!endif
@ondemand linux-*
!ifdef SPLICE
!define SPLICE_ENABLED
!endif
@end
!ifdef SPLICE_ENABLED
@define !UNIX_FLUSH_OPENING
@case linux-* UNWRAP(do { iovec iov{start, (size_t)ptr - (size_t)start}; start += (n_written = vmsplice(STDOUT_FILENO, &iov, 1, SPLICE_F_GIFT)); } while (n_written > 0); if (n_written) { start++;)
@case macos-* {
@end
!define UNIX_FLUSH_CLOSING }
!else
!define UNIX_FLUSH_OPENING
!define UNIX_FLUSH_CLOSING
!endif
@match
@case linux-*,macos-*
        auto start = base;
        ssize_t n_written;
        UNIX_FLUSH_OPENING
        do
            start += (n_written = write(STDOUT_FILENO, start, (char*)ptr - start));
        while (n_written > 0);
        ensure(~n_written)
        UNIX_FLUSH_CLOSING
@case windows-*
        DWORD n_written;
        ensure(WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), base, (char*)ptr - base, &n_written, NULL))
@end
!ifdef INTERACTIVE
        ptr = (NonAliasingChar*)base;
!endif
    }

    void print(char value) {
        *ptr++ = value;
    }
!ifdef CHAR_WITH_SIGN_IS_GLYPH
    void print(uint8_t value) {
        *ptr++ = value;
    }
    void print(int8_t value) {
        *ptr++ = value;
    }
!endif
    void print(bool value) {
        *ptr++ = '0' + value;
    }

    template<typename T, int MinDigits, int MaxDigits, T Factor = 1>
    void write_int_split(T value, T interval) {
        if constexpr (MaxDigits == 1) {
            if (MinDigits || value >= Factor)
                *ptr++ = '0' + interval;
!ifdef LUT
        } else if constexpr (MaxDigits == 2) {
            if (MinDigits >= 2 || value >= 10 * Factor)
                print(decimal_lut[interval * 2]);
            if (MinDigits || value >= Factor)
                print(decimal_lut[interval * 2 + 1]);
!endif
        } else {
            constexpr auto computed = [] {
                int low_digits = 1;
                T coeff = 10;
                while ((low_digits *= 2) < MaxDigits)
                    coeff *= coeff;
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

!ifdef FLOAT
    template<typename T, typename = decltype(T{1.})>
    void print(T value) {
        if (value < 0) {
            print('-');
            value = -value;
        }
        // At least it isn't \write18...
        auto write12 = [&] {
            value *= 1e12;
            write_int_split<uint64_t, 12, 12>(value, value);
        };
        if (!value)
            return print('0');
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
!endif

    void print(const char* value) {
        // We'd prefer strcpy without null terminator here, but perhaps strcpy itself suffices. It's
        // also a builtin in GCC, which means outputting a constant string is going to be optimized
        // into a mov or two!
@match
@case linux-*,macos-*
        ptr = (NonAliasingChar*)stpcpy((char*)ptr, value);
@case windows-*
        // Windows doesn't provide stpcpy, so we have to simulate it (albeit inefficiently) with
        // strlen+mecpy
        size_t length = strlen(value);
        memcpy((char*)ptr, value, length);
        ptr += length;
@end
    }
!ifdef CHAR_WITH_SIGN_IS_GLYPH
    void print(const uint8_t* value) {
        print((char*)value);
    }
    void print(const int8_t* value) {
        print((char*)value);
    }
!endif

    // std::string is inferred from this:
    void print(string_view value) {
        memcpy(ptr, value.data(), value.size());
        ptr += value.size();
    }

!ifdef COMPLEX
    template<typename T>
    void print(complex<T> value) {
        *this << '(' << value.real() << ',' << value.imag() << ')';
    }
!endif

!ifdef BITSET
    template<size_t N>
    SIMD void print(const bitset<N>& value) {
        auto i = N;
        while (i % SIMD_SIZE)
            *ptr++ = '0' + value[--i];
        auto p = (SIMD_TYPE*)ptr;
        i /= SIMD_SIZE;
        while (i) {
@match
@case *-x86_64+avx2
            auto b = _mm256_set1_epi64x(POWERS_OF_TWO);
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
@case *-x86_64+sse4.1
            auto b = _mm_set1_epi64x(POWERS_OF_TWO);
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
@case *-aarch64+neon
            auto vec = (uint8x8_t)vdup_n_u16(((uint16_t*)&value)[--i]);
            *p++ = '0' - vtstq_u8(
                vcombine_u8(vuzp2_u8(vec, vec), vuzp1_u8(vec, vec)),
                (uint8x16_t)vdupq_n_u64(POWERS_OF_TWO)
            );
@case *-x86_64+none,*-aarch64+none
            *p++ = ((BITSET_SHIFT * ((uint8_t*)&value)[--i]) >> 7) & ONE_BYTES | (ONE_BYTES * 0x30);
@end
        }
        ptr = (NonAliasingChar*)p;
    }
!endif

    template<typename T>
    blazingio_ostream& operator<<(const T& value) {
        print(value);
        return *this;
    }

    blazingio_ostream& operator<<(blazingio_ostream& (*func)(blazingio_ostream&)) {
        return func(*this);
    }
};

!ifdef CERR
struct blazingio_ignoreostream {
    template<typename T>
    blazingio_ignoreostream& operator<<(const T& value) {
        return *this;
    }
    blazingio_ignoreostream& operator<<(blazingio_ignoreostream& (*func)(blazingio_ignoreostream&)) {
        return func(*this);
    }
};
!endif

}

namespace std {
    blazingio::blazingio_istream blazingio_cin;
    blazingio::blazingio_ostream blazingio_cout;
!ifdef CERR
    blazingio::blazingio_ignoreostream blazingio_cerr;
!endif

    blazingio::blazingio_istream& getline(blazingio::blazingio_istream& stream, string& value) {
        blazingio::line_t line{value};
        return stream >> line;
    }

!ifdef INTERACTIVE
    blazingio::blazingio_ostream& flush(blazingio::blazingio_ostream& stream) {
        if (__builtin_expect(blazingio_cin.file_size <= 0, 0))
            stream.do_flush();
        return stream;
    }
    blazingio::blazingio_ostream& endl(blazingio::blazingio_ostream& stream) {
        return stream << '\n' << flush;
    }
!else
    blazingio::blazingio_ostream& flush(blazingio::blazingio_ostream& stream) {
        return stream;
    }
    blazingio::blazingio_ostream& endl(blazingio::blazingio_ostream& stream) {
        return stream << '\n';
    }
!endif

!ifdef CERR
    blazingio::blazingio_ignoreostream& endl(blazingio::blazingio_ignoreostream& stream) {
        return stream;
    }
    blazingio::blazingio_ignoreostream& flush(blazingio::blazingio_ignoreostream& stream) {
        return stream;
    }
!endif
}

@match
@case linux-*,macos-*
@case windows-*
LONG vectored_exception_handler(_EXCEPTION_POINTERS* exception_info) {
    auto exception_record = exception_info->ExceptionRecord;
    if (exception_record->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) {
        char* trigger_address = (char*)exception_record->ExceptionInformation[1];
        if ((size_t)(trigger_address - std::blazingio_cout.base) < 0x40000000) {
            ensure(VirtualAlloc(trigger_address, 0x1000000, MEM_COMMIT, PAGE_READWRITE))
            ensure(VirtualAlloc(trigger_address + 0x1000000, 4096, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD))
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
@end

!ifdef LATE_BINDING
#define freopen(...) if (freopen(__VA_ARGS__) == stdin) std::blazingio_cin = blazingio::blazingio_istream{}
!endif

#define cin blazingio_cin
#define cout blazingio_cout

!ifdef CERR
#ifdef ONLINE_JUDGE
#define cerr blazingio_cerr
#define clog blazingio_cerr
#endif
!endif
