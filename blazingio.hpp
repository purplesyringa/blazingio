// #	define AVX2
// #	define SSE41
// #	define LUT
// #	define CHAR_WITH_SIGN_IS_GLYPH
// #	define BITSET
#	define FLOAT
// #	define COMPLEX
// #	define PIPE

#include <array>
#include <atomic>
#	ifdef BITSET
#include <bitset>
#	endif
#	ifdef COMPLEX
#include <complex>
#	endif
#include <cstring>
#include <fcntl.h>
#include <immintrin.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

#	if !defined(AVX2) && !defined(SSE41)
#	define SIMD
#	else
#	ifdef BITSET
#	ifdef AVX2
#define SIMD __attribute__((target("avx2")))
#	else
#define SIMD __attribute__((target("sse4.1")))
#	endif
#	else
#	ifdef AVX2
#	define SIMD __attribute__((target("avx2")))
#	else
#	define SIMD __attribute__((target("sse4.1")))
#	endif
#	endif
#	endif

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
	operator char() const {
		return (char)c;
	}
};

int empty_fd = fileno(tmpfile());

struct blazingio_istream {
	off_t file_size = -1;
	char* base;
	NonAliasingChar* ptr;
	atomic_bool is_ok = true;

	explicit blazingio_istream() {
		// Reserve some memory, but delay actual read until first SIGBUS. This is because we want
		// freopen to work.
		base = (char*)mmap(NULL, 0x1000000000, PROT_READ, MAP_PRIVATE, empty_fd, 0x1000000000);
		ensure(base != MAP_FAILED);
		ptr = (NonAliasingChar*)base;
	}

	void init() {
		struct stat statbuf;
		ensure(fstat(STDIN_FILENO, &statbuf) != -1);
#	ifdef PIPE
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
#	endif
			file_size = statbuf.st_size;
			// Map one more page than necessary so that SIGBUS is triggered soon after the end
			// of file.
			ensure(mmap(base, file_size + 4096, PROT_READ, MAP_PRIVATE | MAP_FIXED, STDIN_FILENO, 0) != MAP_FAILED);
			ensure(madvise(base, file_size, MADV_POPULATE_READ) != -1);
#	ifdef PIPE
		} else {
			size_t alloc_size = 16384;
			ensure(mmap(base, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_POPULATE, -1, 0) != MAP_FAILED);
			file_size = 0;
			ssize_t n_read;
			while ((n_read = read(0, base + file_size, 0x1000000000 - file_size)) > 0) {
				if ((file_size += n_read) == alloc_size) {
					ensure(mmap(base + alloc_size, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_POPULATE, -1, 0) != MAP_FAILED);
					alloc_size *= 2;
				}
			}
			ensure(n_read != -1);
			// We want file_size + 1 more page
			size_t want_alloc_size = ((file_size + 4095) & ~4095) + 4096;
			// We want SIGBUS instead of SIGSEGV, so mmap a file past the end
			ensure(mmap(base + want_alloc_size - 4096, 4096, PROT_READ, MAP_PRIVATE | MAP_FIXED, empty_fd, 0x1000000000) != MAP_FAILED);
			ensure(munmap(base + want_alloc_size, 0x1000000000 - want_alloc_size) != -1);
		}
#	endif
	}

	void on_eof() {
		// Attempt to read beyond end of stdin. This happens either in skip_whitespace, or in a
		// generic input procedure. In the former case, the right thing to do is stop the loop by
		// encountering a non-space character; in the latter case, the right thing to do is to stop
		// the loop by encountering a space character. Something like "\00" works for both cases: it
		// stops (for instance) integer parsing immediately with a zero, and also stops whitespace
		// parsing *soon*.
		char* p = base + ((file_size + 4095) & ~4095);
		ensure(mmap(p, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != MAP_FAILED);
		p[1] = '0';
		is_ok = false;
	}

	// For people writing cie.tie(0);
	void* tie(nullptr_t) {
		return NULL;
	}

	void skip_whitespace() {
		// 0..' ' are not all whitespace, but we only care about well-formed input
		// We expect short runs here, hence no vectorization
		while (*ptr <= ' ') {
			ptr++;
		}
	}

	SIMD void trace_non_whitespace() {
		// We expect long runs here, hence vectorization. Instrinsics break aliasing, and if we
		// interleave ptr modification with SIMD loading, there's going to be an extra memory write
		// on every iteration.
#	ifdef AVX2
		char* p = (char*)ptr;
		__m256i vec;
		while (
			vec = _mm256_cmpgt_epi8(_mm256_set1_epi8(0x21), _mm256_loadu_si256((__m256i*)p)),
			_mm256_testz_si256(vec, vec)
		) {
			p += 32;
		}
		p += __builtin_ctz(_mm256_movemask_epi8(vec));
		ptr = (NonAliasingChar*)p;
#	elif defined(SSE41)
		char* p = (char*)ptr;
		__m128i vec;
		while (
			vec = _mm_cmpgt_epi8(_mm_set1_epi8(0x21), _mm_loadu_si128((__m128i*)p)),
			_mm_testz_si128(vec, vec)
		) {
			p += 16;
		}
		p += __builtin_ctz(_mm_movemask_epi8(vec));
		ptr = (NonAliasingChar*)p;
#	else
		while (*ptr > ' ') {
			ptr++;
		}
#	endif
	}

	SIMD void trace_line() {
		// We expect long runs here, hence vectorization. Instrinsics break aliasing, and if we
		// interleave ptr modification with SIMD loading, there's going to be an extra memory write
		// on every iteration.
#	ifdef AVX2
		char* p = (char*)ptr;
		__m128i mask = _mm_set_epi64x(0x0000ff0000ff0000, 0x00000000000000ff);
		__m256i vec, vec1, vec2;
		while (
			vec = _mm256_loadu_si256((__m256i*)p),
			_mm256_testz_si256(
				vec1 = _mm256_cmpgt_epi8(_mm256_set1_epi8(16), vec),
				vec2 = _mm256_shuffle_epi8(_mm256_set_m128i(mask, mask), vec)
			)
		) {
			p += 32;
		}
		p += __builtin_ctz(_mm256_movemask_epi8(vec1 & vec2));
		ptr = (NonAliasingChar*)p;
#	elif defined(SSE41)
		char* p = (char*)ptr;
		__m128i vec, vec1, vec2;
		while (
			vec = _mm_loadu_si128((__m128i*)p),
			_mm_testz_si128(
				vec1 = _mm_cmpgt_epi8(_mm_set1_epi8(16), vec),
				vec2 = _mm_shuffle_epi8(_mm_set_epi64x(0x0000ff0000ff0000, 0x00000000000000ff), vec)
			)
		) {
			p += 16;
		}
		p += __builtin_ctz(_mm_movemask_epi8(vec1 & vec2));
		ptr = (NonAliasingChar*)p;
#	else
		while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') {
			ptr++;
		}
#	endif
	}

	template<typename T>
	void collect_digits(T& x) {
		while ((*ptr & 0xf0) == 0x30) {
			x = x * 10 + (*ptr++ - '0');
		}
	}

	template<typename T, T = 1>
	void read_arithmetic(T& x) {
		bool negative = is_signed_v<T> && *ptr == '-';
		ptr += negative;
		collect_digits(x);
		if (negative) {
			x = -x;
		}
	}

#	ifdef FLOAT
	template<typename T, typename = decltype(T{1.})>
	void read_arithmetic(T& x) {
		bool negative = *ptr == '-';
		ptr += negative;
		ptr += *ptr == '+';
		auto start = ptr;
		uint64_t n;
		read_arithmetic(n);
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
			read_arithmetic(new_exponent);
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
#	endif

	void read_arithmetic(bool& x) {
		x = *ptr++ == '1';
	}
	void read_arithmetic(char& x) {
		x = *ptr++;
	}

#	ifdef CHAR_WITH_SIGN_IS_GLYPH
	void read_arithmetic(unsigned char& x) {
		x = *ptr++;
	}
	void read_arithmetic(signed char& x) {
		x = *ptr++;
	}
#	endif

	template<typename T, int = numeric_limits<T>::radix>
	blazingio_istream& operator>>(T& value) {
		skip_whitespace();
		read_arithmetic(value);
		return *this;
	}

	blazingio_istream& operator>>(string& value) {
		skip_whitespace();
		auto start = ptr;
		trace_non_whitespace();
		// We know there's no overlap, so avoid doing this for a little bit of performance:
		// value.assign((const char*)start, ptr - start);
		((basic_string<UninitChar>&)value).resize(ptr - start);
		memcpy(value.data(), start, ptr - start);
		return *this;
	}

#	ifdef COMPLEX
	template<typename T>
	blazingio_istream& operator>>(complex<T>& value) {
		skip_whitespace();
		T re, im{};
		if (*ptr == '(') {
			ptr++;
			read_arithmetic(re);
			if (*ptr++ == ',') {
				skip_whitespace();
				read_arithmetic(im);
				ptr++;
			}
		} else {
			read_arithmetic(re);
		}
		value = {re, im};
		return *this;
	}
#	endif

#	ifdef BITSET
	template<size_t N>
	SIMD blazingio_istream& operator>>(bitset<N>& value) {
		skip_whitespace();
		// As we always read N bytes, we might read past the end of the file in case EOF happens.
		// Luckily, we are allowed to overread up to 4095 bytes after EOF (because there's a
		// 4096-page and its second byte is non-whitespace). Therefore, we only have to check for
		// EOF for large enough N, and in this case the overhead is small enough.
		if (N >= 4096 && !*this) {
			return *this;
		}
		size_t i = N;
#	ifdef AVX2
		while (i % 32) {
			value[--i] = *ptr++ == '1';
		}
		char* p = (char*)ptr;
		i /= 32;
		while (i) {
			((uint32_t*)&value)[--i] = __bswap_32(_mm256_movemask_epi8(_mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*)p) << 7, _mm256_set_epi64x(0x18191a1b1c1d1e1f, 0x1011121314151617, 0x08090a0b0c0d0e0f, 0x0001020304050607))));
			p += 32;
		}
		ptr = (NonAliasingChar*)p;
#	elif defined(SSE41)
		while (i % 16) {
			value[--i] = *ptr++ == '1';
		}
		char* p = (char*)ptr;
		i /= 16;
		while (i) {
			((uint16_t*)&value)[--i] = _mm_movemask_epi8(_mm_shuffle_epi8(_mm_loadu_si128((__m128i*)p) << 7, _mm_set_epi64x(0x0001020304050607, 0x08090a0b0c0d0e0f)));
			p += 16;
		}
		ptr = (NonAliasingChar*)p;
#	else
		while (i) {
			value[--i] = *ptr++ == '1';
		}
#	endif
		return *this;
	}
#	endif

	operator bool() const {
		return is_ok;
	}
	bool operator!() const {
		return !is_ok;
	}
};

struct blazingio_ostream {
	off_t file_size = -1;
	char* base;
	NonAliasingChar* ptr;
	int fd;

#	ifdef LUT
	inline static char decimal_lut[200];
#	endif

	blazingio_ostream() {
		// Reserve some memory, but delay actual write until first SIGBUS. This is because we want
		// freopen to work.
		base = (char*)mmap(NULL, 0x1000000000, PROT_READ | PROT_WRITE, MAP_SHARED, empty_fd, 0x1000000000);
		ensure(base != MAP_FAILED);
		ptr = (NonAliasingChar*)base;

#	ifdef LUT
		// The code gets shorter if we initialize LUT here as opposed to during compile time.
		for (int i = 0; i < 100; i++) {
			decimal_lut[i * 2] = '0' + i / 10;
			decimal_lut[i * 2 + 1] = '0' + i % 10;
		}
#	endif
	}
	~blazingio_ostream() {
		if (!file_size) {
			char* p = (char*)ptr;
			ssize_t n_written = 0;
			while (n_written != -1 && base < p) {
				base += (n_written = write(STDOUT_FILENO, base, p - base));
			}
			ensure(n_written != -1);
		} else if (file_size != -1) {
			ensure(ftruncate(STDOUT_FILENO, (char*)ptr - base) != -1);
		}
	}

	void init() {
#	ifdef PIPE
		struct stat statbuf;
		ensure(fstat(STDOUT_FILENO, &statbuf) != -1);
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
#	endif
			file_size = 16384;
			ensure(ftruncate(STDOUT_FILENO, file_size) != -1);
			// We want the file in O_RDWR mode as opposed to O_WRONLY for mmap(MAP_SHARED), so
			// reopen it via procfs.
			fd = open("/dev/stdout", O_RDWR);
			ensure(fd != -1);
			ensure(mmap(base, 0x1000000000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED | MAP_POPULATE, fd, 0) != MAP_FAILED);
#	ifdef PIPE
		} else {
			// Reserve however much space we need, but don't populate it. We'll rely on the kernel
			// to manage it for us.
			ensure(mmap(base, 0x1000000000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0) != MAP_FAILED);
			file_size = 0;
		}
#	endif
	}

	void on_eof() {
		// Attempt to write beyond end of stdout.
		// Double the size of the file
		ensure(ftruncate(STDOUT_FILENO, file_size * 2) != -1);
		// If we want to populate the pages, we have to remap the area.
		ensure(mmap(base + file_size, file_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, file_size) != MAP_FAILED);
		ensure(madvise(base + file_size, file_size, MADV_POPULATE_WRITE) != -1);
		file_size *= 2;
	}

	blazingio_ostream& operator<<(const char& value) {
		*ptr++ = value;
		return *this;
	}
#	ifdef CHAR_WITH_SIGN_IS_GLYPH
	blazingio_ostream& operator<<(const unsigned char& value) {
		*ptr++ = value;
		return *this;
	}
	blazingio_ostream& operator<<(const signed char& value) {
		*ptr++ = value;
		return *this;
	}
#	endif
	blazingio_ostream& operator<<(const bool& value) {
		*ptr++ = '0' + value;
		return *this;
	}

	template<typename T, int MinDigits, int MaxDigits, T Factor = 1>
	void write_int_split(T value, T interval) {
		if constexpr (MaxDigits == 1) {
			if (MinDigits >= 1 || value >= Factor) {
				*ptr++ = '0' + interval;
			}
#	ifdef LUT
		} else if constexpr (MaxDigits == 2) {
			if (MinDigits >= 2 || value >= 10 * Factor) {
				*ptr++ = decimal_lut[interval * 2];
			}
			if (MinDigits >= 1 || value >= Factor) {
				*ptr++ = decimal_lut[interval * 2 + 1];
			}
#	endif
		} else {
			constexpr auto computed = [] {
				int low_digits = 1;
				T coeff = 10;
				while (low_digits * 2 < MaxDigits) {
					low_digits *= 2;
					coeff *= coeff;
				}
				return pair{low_digits, coeff};
			}();
			constexpr int low_digits = computed.first;
			constexpr T coeff = computed.second;
			write_int_split<T, max(0, MinDigits - low_digits), MaxDigits - low_digits, (T)(Factor * coeff)>(value, interval / coeff);
			write_int_split<T, min(MinDigits, low_digits), low_digits, Factor>(value, interval % coeff);
		}
	}

	template<typename T, T = 1>
	blazingio_ostream& operator<<(const T& value) {
		make_unsigned_t<T> abs = value;
		if (value < 0) {
			*ptr++ = '-';
			abs = -abs;
		}
		write_int_split<
			decltype(abs),
			1,
			array{3, 5, 10, 20}[__builtin_ctz(sizeof(value))]
		>(abs, abs);
		return *this;
	}

#	ifdef FLOAT
	template<typename T, typename = decltype(T{1.})>
	blazingio_ostream& operator<<(T value) {
		if (value < 0) {
			*ptr++ = '-';
			value = -value;
		}
		// At least it isn't \write18...
		auto write8 = [&] {
			unsigned n = value * 1e8;
			write_int_split<unsigned, 8, 8>(n, n);
		};
		if (!value) {
			return *this << '0';
		}
		if (value >= 1e16) {
			value *= 1e-16;
			int exponent = 16;
			while (value >= 1) {
				value *= .1;
				exponent++;
			}
			*ptr++ = '0';
			*ptr++ = '.';
			write8();
			*ptr++ = 'e';
			*this << exponent;
		} else if (value >= 1) {
			uint64_t whole = value;
			*this << whole;
			if (value -= whole) {
				*ptr++ = '.';
				write8();
			}
		} else {
			*ptr++ = '0';
			*ptr++ = '.';
			write8();
		}
		return *this;
	}
#	endif

	blazingio_ostream& operator<<(const char* const& value) {
		// We'd prefer strcpy without null terminator here, but perhaps strcpy itself suffices. It's
		// also a builtin in GCC, which means outputting a constant string is going to be optimized
		// into a mov or two!
		ptr = (NonAliasingChar*)stpcpy((char*)ptr, value);
		return *this;
	}
#	ifdef CHAR_WITH_SIGN_IS_GLYPH
	blazingio_ostream& operator<<(const unsigned char* const& value) {
		return *this << (char*)value;
	}
	blazingio_ostream& operator<<(const signed char* const& value) {
		return *this << (char*)value;
	}
#	endif

	// std::string is inferred from this:
	blazingio_ostream& operator<<(const string_view& value) {
		memcpy(ptr, value.data(), value.size());
		ptr += value.size();
		return *this;
	}

#	ifdef COMPLEX
	template<typename T>
	blazingio_ostream& operator<<(const complex<T>& value) {
		return *this << '(' << ' ' << value.real() << ',' << ' ' << value.imag() << ')';
	}
#	endif

#	ifdef BITSET
	template<size_t N>
	SIMD blazingio_ostream& operator<<(const bitset<N>& value) {
		size_t i = N;
#	ifdef AVX2
		while (i % 32) {
			*ptr++ = '0' + value[--i];
		}
		char* p = (char*)ptr;
		i /= 32;
		while (i) {
			_mm256_storeu_si256(
				(__m256i*)p,
				_mm256_sub_epi8(
					_mm256_set1_epi8('0'),
					_mm256_cmpeq_epi8(
						_mm256_shuffle_epi8(
							_mm256_set1_epi32(((uint32_t*)&value)[--i]),
							_mm256_set_epi64x(0x0000000000000000, 0x0101010101010101, 0x0202020202020202, 0x0303030303030303)
						) & _mm256_set1_epi64x(0x0102040810204080),
						_mm256_set1_epi64x(0x0102040810204080)
					)
				)
			);
			p += 32;
		}
		ptr = (NonAliasingChar*)p;
#	elif defined(SSE41)
		while (i % 16 > 0) {
			*ptr++ = '0' + value[--i];
		}
		char* p = (char*)ptr;
		i /= 16;
		while (i) {
			_mm_storeu_si128(
				(__m128i*)p,
				_mm_sub_epi8(
					_mm_set1_epi8('0'),
					_mm_cmpeq_epi8(
						_mm_shuffle_epi8(
							_mm_set1_epi16(((uint16_t*)&value)[--i]),
							_mm_set_epi64x(0x0000000000000000, 0x0101010101010101)
						) & _mm_set1_epi64x(0x0102040810204080),
						_mm_set1_epi64x(0x0102040810204080)
					)
				)
			);
			p += 16;
		}
		ptr = (NonAliasingChar*)p;
#	else
		while (i) {
			*ptr++ = '0' + value[--i];
		}
#	endif
		return *this;
	}
#	endif

	blazingio_ostream& operator<<(blazingio_ostream& (*func)(blazingio_ostream&)) {
		return func(*this);
	}
};

struct blazingio_ignoreostream {
	template<typename T>
	blazingio_ignoreostream& operator<<(const T& value) {
		return *this;
	}
	blazingio_ignoreostream& operator<<(blazingio_ignoreostream& (*func)(blazingio_ignoreostream&)) {
		return func(*this);
	}
};

}

namespace std {
	blazingio::blazingio_istream blazingio_cin;
	blazingio::blazingio_ostream blazingio_cout;
	blazingio::blazingio_ignoreostream blazingio_cerr;

	blazingio::blazingio_istream& getline(blazingio::blazingio_istream& in, string& value) {
		if (*in.ptr) {
			auto start = in.ptr;
			in.trace_line();
			// We know there's no overlap, so avoid doing this for a little bit of performance:
			// value.assign((const char*)start, in.ptr - start);
			((basic_string<blazingio::UninitChar>&)value).resize(in.ptr - start);
			memcpy(value.data(), (char*)start, in.ptr - start);
			in.ptr += *in.ptr == '\r';
			in.ptr++;
		} else {
			in.is_ok = false;
		}
		return in;
	}

	blazingio::blazingio_ostream& endl(blazingio::blazingio_ostream& stream) {
		return stream << '\n';
	}
	blazingio::blazingio_ostream& flush(blazingio::blazingio_ostream& stream) {
		return stream;
	}

	blazingio::blazingio_ignoreostream& endl(blazingio::blazingio_ignoreostream& stream) {
		return stream;
	}
	blazingio::blazingio_ignoreostream& flush(blazingio::blazingio_ignoreostream& stream) {
		return stream;
	}
}

struct init {
	init() {
		struct sigaction act;
		act.sa_sigaction = on_sigbus;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGBUS, &act, NULL);
	}

	static void on_sigbus(int, siginfo_t* info, void*) {
		using namespace std;
		if (info->si_addr == blazingio_cin.base && blazingio_cin.file_size == -1) {
			blazingio_cin.init();
		} else if (info->si_addr == blazingio_cin.base + ((blazingio_cin.file_size + 4095) & ~4095)) {
			blazingio_cin.on_eof();
		} else if ((uintptr_t)info->si_addr - (uintptr_t)(blazingio_cout.base + blazingio_cout.file_size) < 4096) {
			if (blazingio_cout.file_size == -1) {
				blazingio_cout.init();
			} else {
				blazingio_cout.on_eof();
			}
		} else {
			ensure(false);
		}
	}
} blazingio_init;

#define cin blazingio_cin
#define cout blazingio_cout

#ifdef ONLINE_JUDGE
#define cerr blazingio_cerr
#define clog blazingio_cerr
#endif
