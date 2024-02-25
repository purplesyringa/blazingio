#include <array>
!ifdef BITSET
#include <bitset>
!endif
!ifdef COMPLEX
#include <complex>
!endif
#include <cstring>
@include
@case *-x86+avx2,*-x86+sse4.1 <immintrin.h>
@case *-aarch64+neon <arm_neon.h>
@case *-x86+none,*-aarch64+none none
@end
@ondemand windows-*
#include <stdint.h>
@end
@include
@case linux-*,macos-* <sys/mman.h>
@case windows-* <windows.h>
@end
!ifdef INTERACTIVE
#include <sys/stat.h>
!endif
@include
@case linux-*,macos-* <unistd.h>
@case windows-* <io.h>
@end

!define UNSET_SIMD
@ondemand *-x86+avx2,*-x86+sse4.1
!undef UNSET_SIMD
!define UNSET_SIMD #define SIMD
@end

@ondemand windows-*
#ifdef _MSC_VER
#include <__msvc_int128.hpp>
#define int128_t _Signed128
UNSET_SIMD
#else
@end
#define int128_t __int128
@define SIMD
@case *-x86+avx2 __attribute__((target("avx2")))
@case *-x86+sse4.1 __attribute__((target("sse4.1")))
@case *-x86+none,*-aarch64
@end
@ondemand windows-*
#endif
@end

@define SIMD_SIZE
@case *-x86+avx2 32
@case *-x86+sse4.1,*-aarch64+neon 16
@case *-x86+none,*-aarch64+none 8
@end

@define #SIMD_TYPE
@case *-x86+avx2 __m256i
@case *-x86+sse4.1 __m128i
@case *-aarch64+neon uint8x16_t
@case *-x86+none,*-aarch64+none uint64_t
@end

// This is ridiculous but necessary for clang codegen to be at least somewhat reasonable --
// otherwise it resorts to way too many memory accesses. XXX: is this necessary on MSVC?
// MinGW eats up __forceinline just fine
@define INLINE
@case linux-*,macos-* __attribute__((always_inline))
@case windows-* __forceinline
@end

!ifdef INTERACTIVE
#define FETCH fetch(),
!else
!define FETCH
!endif

#define ensure(x) if (!(x)) abort();

@match
@case linux-*,macos-*
@case windows-*
LONG WINAPI vectored_exception_handler(_EXCEPTION_POINTERS*);
@end

// Using unary minus on unsigned numbers triggers an MSVC warning. The fix is rather simple, but
// I'd rather it was isolated so that it could be rolled back if MSVC zero-warning support is
// dropped.
!define NEGATE_MAYBE_UNSIGNED(x) -x
@ondemand windows-*
!undef NEGATE_MAYBE_UNSIGNED
!define NEGATE_MAYBE_UNSIGNED(x) 1 + ~x
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

const uint64_t ONE_BYTES = ~0ULL / 255
!ifdef BITSET
@ondemand *-x86+none,*-aarch64+none
, BITSET_SHIFT = 0x8040201008040201
@end
!endif
;

// Actually 0x0102040810204080
!define POWERS_OF_TWO ~2ULL / 254

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
        off_t file_size =
        // Windows works just fine with lseek, but MSVC throws a warning we'd like to avoid.
        // interactive=n is a rare case, so why not?
@match
@case linux-*,macos-*
        lseek
@case windows-*
        _lseek
@end
        (STDIN_FILENO, 0, SEEK_END);
        ensure(~file_size)
!endif
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
        char* base = (char*)VirtualAlloc(NULL, (file_size + 8191) & -4096, MEM_RESERVE, PAGE_NOACCESS);
        ensure(base)
        ensure(VirtualFree(base, 0, MEM_RELEASE))
        // Map the file there
        DWORD mmaped_region_size = file_size & -65536;
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
        DWORD tmp_n_read = 0;
        ReadFile(GetStdHandle(STD_INPUT_HANDLE), base + mmaped_region_size, 65536, &tmp_n_read, NULL);
        // WIN32_MEMORY_RANGE_ENTRY range{end - file_size, (size_t)file_size};
        // ensure(PrefetchVirtualMemory(GetCurrentProcess(), 1, &range, 0) != -1)
@case linux-*,macos-*
        // We expect a zero byte after EOF. On Linux, man mmap(2) says:
        //     A file is mapped in multiples of the page size.  For a file that is not a multiple of
        //     the page size, the remaining bytes in the partial page at the end of the mapping are
        //     zeroed when mapped, and modifications to that region are not written out to the file.
        // This is not the whole truth: if the file has previously been mapped with MAP_SHARED,
        // modifications to the few bytes after EOF are saved in the shared page and visible even to
        // processes that map the file with MAP_PRIVATE. Therefore assuming the rest of the last
        // page is zero-filled is unreliable and has to be fixed. To do that, we mmap the file
        // read-write and explicitly zero the byte after EOF.
        // Various functions assume at least a few bytes after EOF are readable. For instance,
        // that's what vectorized implementations expect. Round that up to page size for simplicity
        // because we're going to map an anonymous page anyway. This also enables bitset to work
        // more efficiently for bitsets of size up to 4095 on all architectures.
        int page_size = getpagesize();
        char* base = (char*)mmap(NULL, file_size + page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, STDIN_FILENO, 0);
        ensure(base != MAP_FAILED)
        // Remap the last page from anonymous mapping to avoid SIGBUS
        ensure(mmap(base + ((file_size + page_size - 1) & -page_size), page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != MAP_FAILED)
        // Handle attempts to read beyond EOF of stdin gracefully. This would happen either in
        // operator>> while skipping whitespace, or in input(). In the former case, the right thing
        // to do is stop the loop by encountering a non-space character; in the latter case, the
        // right thing to do is to stop the loop by encountering a space character. Something like
        // "\n0" works for both cases: it stops (for instance) integer parsing immediately with a
        // zero, and also stops whitespace parsing *soon*. \n is chosen instead of \0 so that
        // getline can detect EOL by scanning for \n and \r\n without caring about \0.
@end
        end = (NonAliasingChar*)base + file_size;
        *end = '\n';
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
        if (Interactive && ptr == end) {
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
@case linux-*
@case macos-* "x80"
@end
@define !INSN
@case *-x86_64 "syscall"
@case *-i386 "int $128"
@end
@define !ARGS
@case *-x86_64 "+S"(arg1) : "D"
@case *-i386 "+c"(arg1) : "b"
@end
@define !CLOBBERS
@case *-x86_64 UNWRAP(: "rcx", "r11")
@case *-i386
@end
!define UNIX_READ \
@match
@case *-x86 \
            off_t n_read = SYS_read; \
            NonAliasingChar* arg1 = buffer; \
            asm volatile( \
                /* XXX: Handling errors here is complicated, because on Linux syscall will return
                   a small negative number in rax, leading to OOB write, while on XNU the syscall
                   will return a small positive number in rax and set a carry flag we ignore, making
                   it seem like we've just read a few bytes. Neither case is handled correctly, and
                   'ensure(n_read >= 0)' just hides the error, so let us explicitly state we don't
                   support errors returned from read(2) for now. This should probably be fixed
                   later. */ \
                INSN \
                : "+a"(n_read), ARGS(STDIN_FILENO), "d"(65536) \
                CLOBBERS \
            ); \
            ptr = arg1; \
@case *-aarch64 wrap
            /* Linux:  svc 0, syscall number in x8
               Mac OS: svc 0x80, syscall number in x16 */ \
            register long \
                n_read asm("x0") = STDIN_FILENO, \
                arg1 asm("x1") = (long)buffer, \
                arg2 asm("x2") = 65536, \
                syscall_no asm(SYSCALL_NO_REGISTER) = SYS_read; \
            asm volatile( \
                "svc 0" SVC \
                : "+r"(n_read), "+r"(arg1) \
                : "r"(syscall_no), "r"(arg2) \
            ); \
            /* On XNU, x1 is overridden after syscall, so we can't rely on arg1 like
               we did in x86 case. */ \
            ptr = launder(buffer); \
@end
!else
!define UNIX_READ off_t n_read = read(STDIN_FILENO, ptr = buffer, 65536); ensure(~n_read)
!endif
@match
@case linux-*,macos-*
            UNIX_READ
@case windows-*
            DWORD n_read = 0;
            ReadFile(GetStdHandle(STD_INPUT_HANDLE), ptr = buffer, 65536, &n_read, NULL);
@end
            end = ptr + n_read;
            // This matches the behavior we have with files
            *end = '\n';
!ifdef STDIN_EOF
            if (!n_read)
                // This is an attempt to read past EOF. Simulate this just like with files, with
                // "\n0". Be careful to use 'buffer' instead of 'ptr' here -- using the latter
                // confuses GCC's optimizer for some reason.
                buffer[1] = '0',
                // We want ptr == end to evaluate to false.
                end = NULL;
!endif
        }
    }
!endif

    template<typename T>
    INLINE void collect_digits(T& x) {
        while (FETCH (*ptr & 0xf0) == 0x30)
            x = x * 10 + (*ptr++ - '0');
    }

    template<typename T>
    INLINE
    // Without float support, only integers can be cast to 1. With floating-point support, we can't
    // use this trick.
!ifdef FLOAT
    enable_if_t<is_integral_v<T>>
!else
    decltype((void)T{1})
!endif
    input(T& x) {
        bool negative = is_signed_v<T> && (FETCH *ptr == '-');
        ptr += negative;
        collect_digits(x = 0);
        x = negative ? NEGATE_MAYBE_UNSIGNED(x) : x;
    }

!ifdef FLOAT
    template<typename T>
    INLINE decltype((void)T{1.}) input(T& x) {
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
        for (; i < 18 && (FETCH *ptr & 0xf0) == 0x30; i++)
            n = n * 10 + *ptr++ - '0',
            exponent -= has_dot;
        x = (T)n;
        while ((FETCH *ptr & 0xf0) == 0x30)
            x = x * 10 + *ptr++ - '0',
            exponent -= has_dot;
        if (*ptr == '.')
            ptr++,
            has_dot = true;
        while ((FETCH *ptr & 0xf0) == 0x30)
            x = x * 10 + *ptr++ - '0',
            exponent -= has_dot;

        int new_exponent;
        if ((*ptr | 0x20) == 'e')
            ptr++,
            FETCH ptr += *ptr == '+',
            input(new_exponent),
            exponent += new_exponent;

        // This generates {1e-20, 1e-14, ..., 1e14, 1e20}
        static constexpr auto exps = []() {
            array<T, 41> exps{};
            T x = 1;
            for (int i = 21; i--; )
                exps[40 - i] = x,
                exps[i] = 1 / x,
                x *= 10;
            return exps;
        }();

        while (exponent > 40)
            x *= (T)1e10,
            exponent -= 10;
        while (exponent < 0)
            x *= (T)1e-10,
            exponent += 10;
        x *= exps[exponent];

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

    template<typename T>
    SIMD void input_string_like(string& value, T trace) {
!ifdef INTERACTIVE
        fetch();
!endif
        auto start = ptr;
        trace();

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
            trace();
            value.append(buffer, ptr);
        }
!endif
    }

    SIMD void input(string& value) {
        input_string_like(value, [&]() SIMD {
            // We expect long runs here, hence vectorization. Instrinsics break aliasing, and if we
            // interleave ptr modification with SIMD loading, there's going to be an extra memory
            // write on every iteration.
            SIMD_TYPE* p = (SIMD_TYPE*)ptr;
@match
@case linux-*,macos-*
@case windows-*
            ULONG index;
@end
@define !BSFD(x)
@case linux-*,macos-* __builtin_ctz(x)
@case windows-* (_BitScanForward(&index, x), index)
@end
@define !BSFQ_64BIT(x)
@case linux-*,macos-* __builtin_ctzll(x)
@case windows-* (_BitScanForward64(&index, x), index)
@end
@define !BSFQ(x)
@case linux-*,macos-*,windows-x86_64,windows-aarch64 BSFQ_64BIT(x)
@case windows-i386 (_BitScanForward(&index, (ULONG)x) || (_BitScanForward(&index, ULONG(x >> 32)), index += 32), index)
@end
@match
@case *-x86+avx2 wrap
            int mask;
            __m256i space = _mm256_set1_epi8(' ');
            while (
                !(mask = _mm256_movemask_epi8(
                    _mm256_cmpeq_epi8(space, _mm256_max_epu8(space, _mm256_loadu_si256(p)))
                ))
            )
                p++;
            ptr = (NonAliasingChar*)p + BSFD(mask);
@case *-x86+sse4.1 wrap
            int mask;
            __m128i space = _mm_set1_epi8(' ');
            while (
                !(mask = _mm_movemask_epi8(
                    _mm_cmpeq_epi8(space, _mm_max_epu8(space, _mm_loadu_si128(p)))
                ))
            )
                p++;
            ptr = (NonAliasingChar*)p + BSFD(mask);
@case *-aarch64+neon wrap
            uint64x2_t vec;
            while (vec = (uint64x2_t)(*p <= ' '), !(vec[0] | vec[1]))
                p++;
            ptr = (NonAliasingChar*)p + (vec[0] ? 0 : 8) + BSFQ_64BIT(vec[0] ?: vec[1]) / 8;
@case *-x86+none,*-aarch64+none
            // This is a variation on Mycroft's algorithm. See
            // https://groups.google.com/forum/#!original/comp.lang.c/2HtQXvg7iKc/xOJeipH6KLMJ for
            // the original code.
            uint64_t vec;
            while (!(vec = ((*p - ONE_BYTES * 33) & ~*p & (ONE_BYTES << 7))))
                p++;
            ptr = (NonAliasingChar*)p + BSFQ(vec) / 8;
@end
        });
    }

    SIMD void input(line_t& line) {
        input_string_like(line.value, [&]() {
            ptr = (NonAliasingChar*)memchr(ptr, '\n', end - ptr + 1);
        });

        if (line.value.size() && line.value.back() == '\r')
            line.value.pop_back();

        // Skip \n unless the terminator is part of EOF and we have read a non-empty string (so as
        // not to trigger EOF after reading a non-terminated line)
        if (line.value.empty() || ptr < end)
            ptr += *ptr == '\n';
    }

!ifdef COMPLEX
    template<typename T>
    INLINE void input(complex<T>& value) {
        T real_part, imag_part{};
        if (FETCH *ptr == '(') {
            ptr++;
            input(real_part);
            if (FETCH *ptr++ == ',')
!ifdef INTERACTIVE
                rshift_impl(imag_part),
!else
                *this >> imag_part,
!endif
                ptr++;
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
        ptrdiff_t i = N;
!ifdef INTERACTIVE
        while (i)
            if (FETCH i % SIMD_SIZE || end - ptr < SIMD_SIZE)
                value[--i] = *ptr++ == '1';
            else {
!else
        while (i % SIMD_SIZE)
            value[--i] = *ptr++ == '1';
!endif
                auto p = (SIMD_TYPE*)ptr;
!ifdef INTERACTIVE
                for (int64_t j = 0; j < min(i, end - ptr) / SIMD_SIZE; j++) {
!else
                while (i) {
!endif
                    i -= SIMD_SIZE;
@define !BSWAP32
@case linux-*,macos-* __builtin_bswap32
@case windows-* _byteswap_ulong
@end
@match
@case *-x86+avx2
                    // This is actually 0x0001020304050607
                    uint64_t a = ~0ULL / 65025;
                    ((uint32_t*)&value)[i / 32] = BSWAP32(
                        _mm256_movemask_epi8(
                            _mm256_shuffle_epi8(
                                _mm256_slli_epi32(_mm256_loadu_si256(p++), 7),
                                _mm256_set_epi64x(
                                    a + ONE_BYTES * 24,
                                    a + ONE_BYTES * 16,
                                    a + ONE_BYTES * 8,
                                    a
                                )
                            )
                        )
                    );
@case *-x86+sse4.1
                    // This is actually 0x0001020304050607
                    uint64_t a = ~0ULL / 65025;
                    ((uint16_t*)&value)[i / 16] = _mm_movemask_epi8(
                        _mm_shuffle_epi8(
                            _mm_slli_epi32(_mm_loadu_si128(p++), 7),
                            _mm_set_epi64x(a, a + ONE_BYTES * 8)
                        )
                    );
@case *-aarch64+neon wrap
                    auto masked = (uint8x16_t)vdupq_n_u64(POWERS_OF_TWO) & ('0' - *p++);
                    auto zipped = vzip_u8(vget_high_u8(masked), vget_low_u8(masked));
                    ((uint16_t*)&value)[i / 16] = vaddvq_u16(
                        (uint16x8_t)vcombine_u8(zipped.val[0], zipped.val[1])
                    );
@case *-x86+none,*-aarch64+none
                    ((char*)&value)[i / 8] = ((*p++ & ONE_BYTES) * BITSET_SHIFT) >> 56;
@end
                }
                ptr = (NonAliasingChar*)p;
!ifdef INTERACTIVE
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
    istream_impl<false> file;
    istream_impl<true> interactive;

    blazingio_istream() {
        // We want to switch to a pipe-based method if the file is a special device. This cannot be
        // reliably detected by the return value of lseek(SEEK_END) because the returned value
        // depends on the OS:
        // - Linux returns -1, with errno EISPIPE.
        // - Mac OS returns 0.
        // - Windows returns 131072 (wtf), supposedly the size of the buffer.
        // Therefore, don't try to be smart about this and just do an honest stat
        struct stat stat_buf;
        ensure(~fstat(STDIN_FILENO, &stat_buf))
        // A real S_ISREG is sometimes unavailable (e.g. under MSVC), so simulate it
        (stat_buf.st_mode >> 12) == 8
            ? file.init_assume_file(stat_buf.st_size)
            : interactive.init_assume_interactive();
    }

    // For people writing cie.tie(0);
    void* tie(nullptr_t) {
        return NULL;
    }

    template<typename T>
    INLINE blazingio_istream& operator>>(T& value) {
        file.ptr
            ? file.rshift_impl(value)
            : interactive.rshift_impl(value);
        return *this;
    }

!ifdef STDIN_EOF
    operator bool() {
        return !!*this;
    }
    bool operator!() {
        return file.ptr ? !file : !interactive;
    }
!endif
};
!endif

uint16_t decimal_lut[100];
char max_digits_by_log2[64]{1};

struct SPLIT_HERE blazingio_ostream {
    char* base;
    NonAliasingChar* ptr;
    bool ever_flushed;

    blazingio_ostream() {
        // We *could* use 'base = new char[0x20000000];' instead of mmap-based allocation here, but
        // that would lead to problems on systems without overcommit, such as Windows.
        // The size is limited by a bit greater than 0x20000000 because 32-bit WINE only allows to
        // allocate that much.
@match
@case linux-*,macos-*
        // Avoid MAP_SHARED: it turns out it's pretty damn inefficient compared to a write at the
        // end. This also allows us to allocate memory immediately without waiting for freopen,
        // because we'll only use the fd in the destructor.
        base = (char*)mmap(
            NULL,
            0x20000000,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
            -1,
            0
        );
        ensure(base != MAP_FAILED)
@case windows-*
        // Windows doesn't support anything like MAP_NORESERVE or overcommit. Therefore, reserve
        // memory and use guard pages to extend the committed region.
        ensure(base = (char*)VirtualAlloc(NULL, 0x20000000, MEM_RESERVE, PAGE_READWRITE))
        ensure(VirtualAlloc(base, 4096, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD))
        AddVectoredExceptionHandler(true, vectored_exception_handler);
@end
        ptr = (NonAliasingChar*)base;
        // The code gets shorter if we initialize LUT here as opposed to during compile time.
        for (int i = 0; i < 100; i++)
            decimal_lut[i] = ('0' + i / 10) | (('0' + i % 10) << 8);
        for (int i = 1; i < 64; i++)
            max_digits_by_log2[i] = max_digits_by_log2[i - 1] + (0x8922489224892249 >> i & 1);
    }
    ~blazingio_ostream() {
!ifdef INTERACTIVE
        flush(
@ondemand windows-*
           !ever_flushed
@end
        );
    }
    void flush(
@ondemand windows-*
        int attempt_direct_write = false
@end
    ) {
!endif
@match
@case linux-*,macos-*
        auto start = base;
        ssize_t n_written;
        while ((n_written = write(STDOUT_FILENO, start, (char*)ptr - start)) > 0)
            start += n_written;
        ensure(~n_written)
@case windows-*
        ever_flushed = true;
        auto stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
!define WRAP_REOPEN(x) x
!ifdef INTERACTIVE
!undef WRAP_REOPEN
!define WRAP_REOPEN(x) attempt_direct_write ? x : INVALID_HANDLE_VALUE
!endif
        auto handle = WRAP_REOPEN(ReOpenFile(
            stdout_handle,
            GENERIC_WRITE,
            // Be as general as possible
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH
        ));
        DWORD n_written;
        ensure(
            handle == INVALID_HANDLE_VALUE
                ? WriteFile(stdout_handle, base, DWORD((char*)ptr - base), &n_written, NULL)
                : (
                    WriteFile(handle, base, DWORD(((char*)ptr - base + 4095) & -4096), &n_written, NULL)
                    && ~_chsize(1, int((char*)ptr - base))
                )
        )
@end
!ifdef INTERACTIVE
        ptr = (NonAliasingChar*)base;
!endif
    }
!ifndef INTERACTIVE
    void flush() {}
!endif

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

    template<typename T>
    // We can't use decltype((void)T{1}) here because that's going to conflict with std::string.
    enable_if_t<is_integral_v<T>> print(T value) {
        using AbsT = make_unsigned_t<T>;

        AbsT abs = value;
        if (value < 0)
            print('-'),
            abs = NEGATE_MAYBE_UNSIGNED(abs);

!ifndef CHAR_WITH_SIGN_IS_GLYPH
        if constexpr (sizeof(T) == 1) {
            int digits = 1 + (abs >= 10) + (abs >= 100);
            NonAliasingChar buf[6];
            memcpy(buf, decimal_lut + abs / 10, 2);
            buf[2] = '0' + abs % 10;
            memcpy(ptr, buf + 3 - digits, 4);
            ptr += digits;
            return;
        }
!endif

        static constexpr auto powers_of_ten = []() {
            array<AbsT, 5 * sizeof(T) / 2> powers_of_ten{};
            AbsT n = 1;
            for (size_t i = 1; i < powers_of_ten.size(); i++)
                n *= 10,
                powers_of_ten[i] = n;
            return powers_of_ten;
        }();

        // We somehow need to skip leading zeroes. Do that by computing decimal length separately.
@match
@case linux-*,macos-*
@case windows-*
        ULONG ilog2;
@end
        int digits = max_digits_by_log2[
@define !BSRQ_WINDOWS
@case *-x86_64,*-aarch64 _BitScanReverse64(&ilog2, abs | 1)
@case *-i386 _BitScanReverse(&ilog2, ULONG((int64_t)abs >> 32)) ? ilog2 += 32 : _BitScanReverse(&ilog2, (ULONG)abs | 1)
@end
@match
@case windows-*
            (BSRQ_WINDOWS, ilog2)
@case linux-*,macos-*
            // This compiles to a single instruction on x64. |1 is to handle abs == 0 gracefully.
            63 ^ __builtin_clzll(abs | 1)
@end
        ];
        digits -= abs < powers_of_ten[digits - 1];

        // This is a variation on Terje Mathisen's algorithm. See
        // http://computer-programming-forum.com/46-asm/7aa4b50bce8dd985.htm

        uint16_t buf[20];

        if constexpr (sizeof(T) == 2) {
            // We use a 32-bit fixed-point format here. The high 7 bits are the whole part and the
            // low 25 low bits are the real part. 7 bits are used because that's the shortest amount
            // of bits 99 fits in.

            // abs / 1e3 in fixed point. 2^25 / 1e3 is actually 33554.432, but rounding it up to
            // 33555 would introduce too big an error. We compensate for it by effectively using
            // 33554.5 as the factor. The computed value, when multiplied back by 1e3, will have the
            // whole part equal to (33554.5e3 * abs) >> 25. We want this to be less than 1 far from
            // abs, i.e.
            //     ((33554.5e3 * abs) >> 25) - abs < 1,
            // or
            //     (33554.5e3 - 2^25) * abs < 2^25.
            // Luckily, this is true from all abs up to 2^16.
            // The computation is a bit off for odd abs: in this case n is 1/2 larger than the
            // theoretical value, which is a ridiculously small error, so the check still passes.
            auto n = 33555U * abs - abs / 2;
            uint64_t buf = decimal_lut[n >> 25];
            n = (n & 0x01ffffff) * 25;
            buf |= decimal_lut[n >> 23] << 16;
            buf |= uint64_t('0' + (((n & 0x007fffff) * 5) >> 22)) << 32;
            buf >>= 40 - digits * 8;
            memcpy(ptr, &buf, 8);
        } else if constexpr (sizeof(T) == 4) {
@match
@case *-x86_64,*-aarch64 wrap
            // We use a 64-bit fixed-point format here. The high 7 bits are the whole part and the
            // low 57 low bits are the rneal part. 7 bits are used because that's the shortest
            // amount of bits 99 fits in.

            // abs / 1e8 in fixed point. 2^57 / 1e8 is actually 1441151880.7585588..., so we round
            // it up. This introduces an error. The computed value, when multiplied back by 1e8,
            // will have the whole part equal to (1441151881e8 * abs) >> 57. We want this to be less
            // than 1 far from abs, i.e.
            //     ((1441151881e8 * abs) >> 57) - abs < 1,
            // or
            //     (1441151881e8 - 2^57) * abs < 2^57.
            // Luckily, this is true from all abs up to 2^32.
            auto n = 1441151881ULL * abs;

            int shift = 57;
            auto mask = ~0ULL >> 7;
            for (int i = 0; i < 5; i++)
                buf[i] = decimal_lut[n >> shift],
                n = (n & mask) * 25,
                shift -= 2,
                mask >>= 2;
@case *-i386 wrap
            // Repeat what's going on in the above case, but use 32.32 for actual computations.
            // Proof of correctness: we want
            //     (((((1441151881 * abs) >> 25) + 1) * 1e8) >> 32) - abs < 1,
            // i.e.
            //     (((1441151881 * abs) >> 25) + 1) * 1e8 < (abs + 1) * 2^32,
            // which follows from
            //     (1441151881e8 - 2^57) * abs < 2^57 - 1e8 * 2^25,
            // which holds for abs up to 2^32.
            auto n = ((1441151881ULL * abs) >> 25) + 1;
            for (int i = 0; i < 5; i++)
                buf[i] = decimal_lut[n >> 32],
                n = (n & ~0U) * 100;
@end

            // Always copying 16 bytes enables us to always mov xmmword as opposed to multiple
            // instructions.
            memcpy(ptr, (NonAliasingChar*)buf + 10 - digits, 16);
        } else /* if constexpr (sizeof(T) == 8) */ {
@match
@case *-x86_64,*-aarch64 wrap
            // This part is also based on James Anhalt's algorithm. See
            // https://jk-jeon.github.io/posts/2022/02/jeaiii-algorithm/

            // We use a 128-bit fixed-point format here. 7.121 would be inefficient because there
            // are no 128-bit operations in x86, but 64.64 works just fine because there is mul r64,
            // and extracting the whole and real parts is just a question of using this or that
            // register.
            //
            // We want to compute abs / 1e18 in fixed point. We could compute that as
            //     abs * (2^64 // 1e18 + 1),
            // but the approximation of 1e-18 in 64.64 is way too imprecise for large 'abs': it's
            // just 19. However, is that not too few bits compared to the seemingly suficient 64?
            // Indeed, the cause of the problem is not the precision of the fixed-point format, but
            // double rounding. If 1e-18 itself wasn't rounded initially, the accuracy would be much
            // better.
            //
            // We shall approximate 'abs * 2^64 * 1e-18' as
            //     ((abs * (2^128 // 1e18 + 1)) >> 64) + 1
            // This is clearly an upper bound. How precise is it? Just like in the 32-bit case, we
            // want
            //     (((((abs * 340282366920938463464) >> 64) + 1) * 1e18) >> 64) - abs < 1
            // This translates to
            //     (((abs * 340282366920938463464) >> 64) + 1) * 1e18 < (abs + 1) * 2^64,
            // which follows from
            //     (abs * 340282366920938463464 / 2^64 + 1) * 1e18 < (abs + 1) * 2^64,
            // which is clearly true because
            //     abs * (340282366920938463464e18 - 2^128) < 2^128 - 2^64 * 1e18
            // holds for all abs up to 2^64. In fact, the left-hand size is just 4% of RHS. We
            // prefer 2^128 to slightly lower powers because this enables us to replace right-shift
            // by 64 with a single register read.
            auto n = int128_t{18} * abs + ((int128_t{8240973594166534376} * abs) >> 64) + 1;

            for (int i = 0; i < 10; i++)
                buf[i] = decimal_lut[int(n >> 64)],
                n = (n & ~0ULL) * 100;
@case *-i386 wrap
            // The i386 case is hard. We don't even have i128; any attempt to emulate it is going to
            // be darn slow. We can't really output longs *fast* though, so this part of code is
            // mostly optimized on a best-effort basis; it's mostly here for feature parity.

            // Guard against people using long long where int suffices
            if (abs < (1ULL << 32)) {
                return print((uint32_t)abs);
            }

            // We'll first perform three divisions to split the number as follows:
            //     01234|56789|01234|56789
            // Each of the groups can then be resolved in 7.25 fixed point numbers.
            auto a_coeff = (uint64_t)1e10;
            auto x = abs / a_coeff, y = abs % a_coeff;
            int b_coeff = 100000, b[] {
                int(x / b_coeff),
                int(x % b_coeff),
                int(y / b_coeff),
                int(y % b_coeff)
            };

            // We wish to compute b / 1e4 in 7.25 fixed point. The precision of the
            //     2^25 / 1e4 = 3355.4432
            // approximation is way too low, so we compute it in 32.32 initially. For
            // correctness we want
            //     (((((429497 * b) >> 7) + 1) * 1e4) >> 25) - b < 1,
            // which reduces to
            //     (((429497 * b) >> 7) + 1) * 1e4 < (1 + b) * 2^25,
            // which follows from
            //     (429497e4 - 2^32) * b < 2^32 - 1e4 * 2^7,
            // which is true for b up to 1e5 (and 1e6, really).
            NonAliasingChar buf[40];
            for (int i = 0; i < 4; i++) {
                uint32_t n = ((429497ULL * b[i]) >> 7) + 1;
                NonAliasingChar* p = buf + i * 5;
                *p = '0' + (n >> 25);
                n = (n & (~0U >> 7)) * 25;
                memcpy(p + 1, decimal_lut + (n >> 23), 2);
                memcpy(p + 3, decimal_lut + (((n & (~0U >> 9)) * 25) >> 21), 2);
            }
@end

            // Always copying 20 bytes enables us to always mov xmmword+r32 as opposed to multiple
            // instructions.
            memcpy(ptr, (NonAliasingChar*)buf + 20 - digits, 20);
        }

        ptr += digits;
    }

!ifdef FLOAT
    template<typename T>
    decltype((void)T{1.}) print(T value) {
        if (value < 0)
            print('-'),
            value = -value;
        // At least it isn't \write18...
        auto write12 = [&]() {
            auto x = uint64_t(value * 1e12);

@match
@case *-x86_64,*-aarch64 wrap
            // This is a variation on Terje Mathisen's algorithm, just like in integer output. The
            // reason for yet another reimplementation in this lambda as opposed to reusing existing
            // code is because 'x' contains just 12 digits, not 20 supported by the general
            // implementation, and that lets us use a less precise approximation.

            // We shall approximate 'abs * 2^64 * 1e-10' as
            //     ((abs * (2^72 // 1e10 + 1)) >> 8) + 1
            // This is clearly an upper bound. To be correct, we want
            //     (((((abs * 472236648287) >> 8) + 1) * 1e10) >> 64) - abs < 1
            // This translates to
            //     (((abs * 472236648287) >> 8) + 1) * 1e10 < (abs + 1) * 2^64
            // which follows from
            //     (abs * 472236648287 / 2^8 + 1) * 1e10 < (abs + 1) * 2^64
            // which is clearly true because
            //     abs * (472236648287e10 - 2^72) < 2^72 - 2^8 * 1e10
            // holds for all abs up to 1e12. We choose 72 as the initial precision instead of
            // something bigger to reduce length of the constant in source code.
            auto n = (int128_t{472236648287} * x >> 8) + 1;
            for (int i = 0; i < 6; i++)
                memcpy(ptr, decimal_lut + int(n >> 64), 2),
                ptr += 2,
                n = (n & ~0ULL) * 100;
@case *-i386 wrap
            // We can't reliably force MSVC to use SSE from within the program, so we have to
            // gracefully handle the case when FPU is used for floating-point computations. There is
            // a minor issue here: if, at any point, 'value' is just under 1 in 80-bit precision,
            // and it is rounded to 1 in 64-bit precision, 'x' is going to be equal to 1e12 exactly,
            // and this breaks stringification. Fix this by introducing a small error to make the
            // number fit in 12 decimal digits anyway.
            x -= x == 1000000000000;

            // Split the 12-digit integer into two 6-digit parts. Then for each part x, apply the
            // same algorithm as the one used in u64.
            uint32_t n[] {
                uint32_t((x / 1000000 * 429497) >> 7) + 1,
                uint32_t((x % 1000000 * 429497) >> 7) + 1
            };
            int shift = 25, mask = ~0U >> 7;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 2; j++)
                    memcpy(ptr + i * 2 + j * 6, decimal_lut + (n[j] >> shift), 2),
                    n[j] = (n[j] & mask) * 25;
                shift -= 2;
                mask >>= 2;
            }
            ptr += 12;
@end
        };
        if (!value)
            return print('0');
        if (value >= 1e16) {
            value *= (T)1e-16;
            int exponent = 16;
            while (value >= 1)
                value *= (T).1,
                exponent++;
            print("0.");
            write12();
            print('e');
            print(exponent);
        } else if (value >= 1) {
            auto whole = (uint64_t)value;
            print(whole);
            if (value -= whole)
                print('.'),
                write12();
        } else
            print("0."),
            write12();
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
        // strlen+memcpy
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
@case *-x86+avx2
            auto b = _mm256_set1_epi64x(POWERS_OF_TWO);
            _mm256_storeu_si256(
                p++,
                _mm256_sub_epi8(
                    _mm256_set1_epi8('0'),
                    _mm256_cmpeq_epi8(
                        _mm256_and_si256(
                            _mm256_shuffle_epi8(
                                _mm256_set1_epi32(((uint32_t*)&value)[--i]),
                                _mm256_set_epi64x(0, ONE_BYTES, ONE_BYTES * 2, ONE_BYTES * 3)
                            ),
                            b
                        ),
                        b
                    )
                )
            );
@case *-x86+sse4.1
            auto b = _mm_set1_epi64x(POWERS_OF_TWO);
            _mm_storeu_si128(
                p++,
                _mm_sub_epi8(
                    _mm_set1_epi8('0'),
                    _mm_cmpeq_epi8(
                        _mm_and_si128(
                            _mm_shuffle_epi8(
                                _mm_set1_epi16(((uint16_t*)&value)[--i]),
                                _mm_set_epi64x(0, ONE_BYTES)
                            ),
                            b
                        ),
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
@case *-x86+none,*-aarch64+none
            *p++ = ((BITSET_SHIFT * (((uint8_t*)&value)[--i]) >> 7) & ONE_BYTES) | (ONE_BYTES * 0x30);
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
        if (!blazingio_cin.file.ptr)
            stream.flush();
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
LONG WINAPI vectored_exception_handler(_EXCEPTION_POINTERS* exception_info) {
    auto exception_record = exception_info->ExceptionRecord;
    char* trigger_address = (char*)exception_record->ExceptionInformation[1];
    if (
        exception_record->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION
        && (size_t)(trigger_address - std::blazingio_cout.base) < 0x40000000
    ) {
        ensure(
            VirtualAlloc(trigger_address, 0x1000000, MEM_COMMIT, PAGE_READWRITE)
            && VirtualAlloc(trigger_address + 0x1000000, 4096, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD)
        )
        return EXCEPTION_CONTINUE_EXECUTION;
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
