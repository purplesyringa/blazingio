#pragma once

#define AVX2

#include <atomic>
#include <bitset>
#include <complex>
#include <cstring>
#include <fcntl.h>
#include <immintrin.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

#ifdef AVX2
#define SIMD __attribute__((target("avx2")))
#else
#define SIMD __attribute__((target("sse4.1")))
#endif

namespace fastio {

struct UninitChar { UninitChar& operator=(const UninitChar&) { return *this; } };

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

struct fastio_istream {
	off_t file_size = -1;
	const char* base;
	const NonAliasingChar* ptr;
	std::atomic_bool is_ok = true;
	int fd;

	explicit fastio_istream(int fd) : fd(fd) {
		// Reserve some memory, but delay actual read until first SIGBUS. This is because we want
		// freopen to work.
		int fd_exe = open("/proc/self/exe", O_RDONLY);
		if (fd_exe == -1) {
			throw std::invalid_argument("invalid io");
		}
		base = (const char*)mmap(NULL, 0x1000000000, PROT_READ, MAP_PRIVATE, fd_exe, 0x1000000000);
		if (base == MAP_FAILED) {
			throw std::invalid_argument("invalid io");
		}
		close(fd_exe);
		ptr = (NonAliasingChar*)base;
	}

	void init() {
		struct stat statbuf;
		if (fstat(fd, &statbuf) == -1) {
			_exit(1);
		}
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
			file_size = statbuf.st_size;
			// Map one more page than necessary so that SIGBUS is triggered soon after the end
			// of file.
			if (mmap((void*)base, file_size + 4096, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED) {
				_exit(1);
			}
			if (madvise((void*)base, file_size, MADV_POPULATE_READ) == -1) {
				_exit(1);
			}
		} else {
			size_t alloc_size = 16384;
			if (mmap((void*)base, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_POPULATE, -1, 0) == MAP_FAILED) {
				_exit(1);
			}
			file_size = 0;
			ssize_t n_read;
			while ((n_read = read(0, (void*)(base + file_size), 0x1000000000 - file_size)) > 0) {
				if ((file_size += n_read) == alloc_size) {
					if (mmap((void*)(base + alloc_size), alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_POPULATE, -1, 0) == MAP_FAILED) {
						_exit(1);
					}
					alloc_size *= 2;
				}
			}
			if (n_read < -1) {
				_exit(1);
			}
			// We want file_size + 1 more page
			size_t want_alloc_size = ((file_size + 4095) & ~4095) + 4096;
			// We want SIGBUS instead of SIGSEGV, so mmap a file past the end
			int fd_exe = open("/proc/self/exe", O_RDONLY);
			if (fd_exe == -1) {
				_exit(1);
			}
			if (mmap((void*)(base + want_alloc_size - 4096), 4096, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd_exe, 0x1000000000) == MAP_FAILED) {
				_exit(1);
			}
			if (munmap((void*)(base + want_alloc_size), 0x1000000000 - want_alloc_size) == -1) {
				_exit(1);
			}
			close(fd_exe);
		}
	}

	void on_eof() {
		// Attempt to read beyond end of stdin. This happens either in skip_whitespace, or in a
		// generic input procedure. In the former case, the right thing to do is stop the loop by
		// encountering a non-space character; in the latter case, the right thing to do is to stop
		// the loop by encountering a space character. Something like "\00" works for both cases: it
		// stops (for instance) integer parsing immediately with a zero, and also stops whitespace
		// parsing *soon*.
		char* p = (char*)base + ((file_size + 4095) & ~4095);
		if (mmap(p, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED) {
			_exit(1);
		}
		p[1] = '0';
		is_ok = false;
	}

	// For people writing cie.tie(0);
	void* tie(std::nullptr_t) {
		return nullptr;
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
		const char* p = (const char*)ptr;
#ifdef AVX2
		__m256i vec;
		do {
			vec = _mm256_cmpgt_epi8(_mm256_set1_epi8(' ' + 1), _mm256_loadu_si256((const __m256i*)p));
			p += 32;
		} while (_mm256_testz_si256(vec, vec));
		p -= 32;
		p += __builtin_ctz(_mm256_movemask_epi8(vec));
#else
		__m128i vec;
		do {
			vec = _mm_cmpgt_epi8(_mm_set1_epi8(' ' + 1), _mm_loadu_si128((const __m128i*)p));
			p += 16;
		} while (_mm_testz_si128(vec, vec));
		p -= 16;
		p += __builtin_ctz(_mm_movemask_epi8(vec));
#endif
		ptr = (const NonAliasingChar*)p;
		// while (*ptr > ' ') {
		// 	ptr++;
		// }
	}

	SIMD void trace_line() {
		// We expect long runs here, hence vectorization. Instrinsics break aliasing, and if we
		// interleave ptr modification with SIMD loading, there's going to be an extra memory write
		// on every iteration.
		const char* p = (const char*)ptr;
		__m128i mask = _mm_set_epi8(0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1);
#ifdef AVX2
		__m256i vec1, vec2;
		do {
			__m256i vec = _mm256_loadu_si256((const __m256i*)p);
			vec1 = _mm256_cmpgt_epi8(_mm256_set1_epi8(16), vec);
			vec2 = _mm256_shuffle_epi8(_mm256_set_m128i(mask, mask), vec);
			p += 32;
		} while (_mm256_testz_si256(vec1, vec2));
		p -= 32;
		p += __builtin_ctz(_mm256_movemask_epi8(_mm256_and_si256(vec1, vec2)));
#else
		__m128i vec1, vec2;
		do {
			__m128i vec = _mm_loadu_si128((const __m128i*)p);
			vec1 = _mm_cmpgt_epi8(_mm_set1_epi8(16), vec);
			vec2 = _mm_shuffle_epi8(mask, vec);
			p += 16;
		} while (_mm_testz_si128(vec1, vec2));
		p -= 16;
		p += __builtin_ctz(_mm_movemask_epi8(_mm_and_si128(vec1, vec2)));
#endif
		ptr = (const NonAliasingChar*)p;
		// while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') {
		// 	ptr++;
		// }
	}

	template<typename T>
	void collect_digits(T& x) {
		while ((*ptr & 0xf0) == 0x30) {
			x = x * 10 + (*ptr++ - '0');
		}
	}

	template<typename T>
	static constexpr T exps[] = {
		1e-15, 1e-14, 1e-13, 1e-12, 1e-11, 1e-10, 1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1,
		1,
		1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15
	};

	template<typename T>
	T read_arithmetic() {
		if constexpr (std::is_same_v<T, char> || std::is_same_v<T, unsigned char> || std::is_same_v<T, signed char>) {
			return *ptr++;
		} else if constexpr (std::is_same_v<T, bool>) {
			return *ptr++ == '1';
		}
		bool negative = std::is_signed_v<T> && *ptr == '-';
		ptr += negative;
		T x;
		if constexpr (std::is_floating_point_v<T>) {
			ptr += *ptr == '+';
			auto start = ptr;
			auto n = read_arithmetic<unsigned long long>();
			int exponent = 0;
			if (*ptr == '.') {
				auto after_dot = ++ptr;
				collect_digits(n);
				exponent = after_dot - ptr;
			}
			if (__builtin_expect(ptr - start >= 19, 0)) {
				ptr = start;
				x = 0;
				collect_digits(x);
				if (*ptr == '.') {
					ptr++;
					collect_digits(n);
				}
			} else {
				x = n;
			}
			if ((*ptr | 0x20) == 'e') {
				ptr++;
				ptr += *ptr == '+';
				exponent += read_arithmetic<int>();
			}
			if (-15 <= exponent && exponent <= 15) {
				x *= exps<T>[exponent + 15];
			} else {
				while (exponent > 0) {
					exponent--;
					x *= 10;
				}
				while (exponent < 0) {
					exponent++;
					x *= 0.1;
				}
			}
			return negative ? -x : x;
		} else {
			x = 0;
			collect_digits(x);
		}
		return negative ? -x : x;
	}

	template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	fastio_istream& operator>>(T& value) {
		skip_whitespace();
		value = read_arithmetic<T>();
		return *this;
	}

	fastio_istream& operator>>(std::string& value) {
		skip_whitespace();
		auto start = ptr;
		trace_non_whitespace();
		// We know there's no overlap, so avoid doing this for a little bit of performance:
		// value.assign((const char*)start, ptr - start);
		((std::basic_string<UninitChar>&)value).resize(ptr - start);
		std::memcpy(value.data(), (const char*)start, ptr - start);
		return *this;
	}

	template<typename T>
	fastio_istream& operator>>(std::complex<T>& value) {
		skip_whitespace();
		if (*ptr == '(') {
			ptr++;
			T real = read_arithmetic<T>();
			T imag{};
			if (*ptr++ == ',') {
				skip_whitespace();
				imag = read_arithmetic<T>();
				ptr++;
			}
			value = {real, imag};
		} else {
			value = read_arithmetic<T>();
		}
		return *this;
	}

	template<size_t N>
	SIMD fastio_istream& operator>>(std::bitset<N>& value) {
		skip_whitespace();
		// As we always read N bytes, we might read past the end of the file in case EOF happens.
		// Luckily, we are allowed to overread up to 4095 bytes after EOF (because there's a
		// 4096-page and its second byte is non-whitespace). Therefore, we only have to check for
		// EOF for large enough N, and in this case the overhead is small enough.
		if (N >= 4096 && !*this) {
			return *this;
		}
		size_t i = N;
		while (i % 32 > 0) {
			value[--i] = *ptr++ == '1';
		}
		const char* p = (const char*)ptr;
#ifdef AVX2
		while (i >= 32) {
			((uint32_t*)&value)[(i -= 32) / 32] = __builtin_bswap32(_mm256_movemask_epi8(_mm256_shuffle_epi8(_mm256_slli_epi16(_mm256_loadu_si256((const __m256i*)p), 7), _mm256_set_epi8(24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19, 20, 21, 22, 23, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7))));
			p += 32;
		}
#else
		while (i >= 16) {
			((uint16_t*)&value)[(i -= 16) / 16] = _mm_movemask_epi8(_mm_shuffle_epi8(_mm_slli_epi16(_mm_loadu_si128((const __m128i*)p), 7), _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)));
			p += 16;
		}
#endif
		ptr = (const NonAliasingChar*)p;
		return *this;
	}

	operator bool() const {
		return is_ok;
	}
	bool operator!() const {
		return !is_ok;
	}
};

struct fastio_ostream {
	off_t file_size = -1;
	char* base;
	NonAliasingChar* ptr;
	int fd;

	fastio_ostream(int fd) : fd(fd) {
		// Reserve some memory, but delay actual write until first SIGBUS. This is because we want
		// freopen to work.
		FILE *f = tmpfile();
		if (f == NULL) {
			throw std::invalid_argument("invalid io");
		}
		base = (char*)mmap(NULL, 0x1000000000, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(f), 0x1000000000);
		if (base == MAP_FAILED) {
			throw std::invalid_argument("invalid io");
		}
		fclose(f);
		ptr = (NonAliasingChar*)base;
	}
	~fastio_ostream() {
		if (file_size == 0) {
			const char* p = (const char*)ptr;
			ssize_t n_written = 0;
			while (n_written != -1 && base < p) {
				base += (n_written = write(fd, base, p - base));
			}
			if (n_written == -1) {
				std::terminate();
			}
		} else if (file_size != -1) {
			if (ftruncate(fd, (char*)ptr - base) == -1) {
				std::terminate();
			}
		}
	}

	void init() {
		struct stat statbuf;
		if (fstat(fd, &statbuf) == -1) {
			_exit(1);
		}
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
			file_size = 16384;
			if (ftruncate(fd, file_size) == -1) {
				_exit(1);
			}
			// We want the file in O_RDWR mode as opposed to O_WRONLY for mmap(MAP_SHARED), so
			// reopen it via procfs.
			char path[20];
			std::sprintf(path, "/proc/self/fd/%d", fd);
			fd = open(path, O_RDWR);
			if (fd == -1) {
				_exit(1);
			}
			if (mmap(base, 0x1000000000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED | MAP_POPULATE, fd, 0) == MAP_FAILED) {
				_exit(1);
			}
		} else {
			// Reserve however much space we need, but don't populate it. We'll rely on the kernel
			// to manage it for us.
			if (mmap(base, 0x1000000000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0) == MAP_FAILED) {
				_exit(1);
			}
			file_size = 0;
		}
	}

	void on_eof() {
		// Attempt to write beyond end of stdout.
		if (
			// Double the size of the file
			ftruncate(fd, file_size * 2) == -1
			// If we want to populate the pages, we have to remap the area.
			|| mmap(base + file_size, file_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, file_size) == MAP_FAILED
			|| madvise(base + file_size, file_size, MADV_POPULATE_WRITE) == -1
		) {
			_exit(1);
		}
		file_size *= 2;
	}

	fastio_ostream& operator<<(const char& value) {
		*ptr++ = value;
		return *this;
	}
	fastio_ostream& operator<<(const unsigned char& value) {
		*ptr++ = value;
		return *this;
	}
	fastio_ostream& operator<<(const signed char& value) {
		*ptr++ = value;
		return *this;
	}
	fastio_ostream& operator<<(const bool& value) {
		*ptr++ = '0' + value;
		return *this;
	}

	static constexpr char decimal_lut[] = "00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";

	template<typename T, T Factor, int MinDigits, int MaxDigits>
	void write_int_split(T value, T interval) {
		if constexpr (MaxDigits == 1) {
			if (MinDigits >= 1 || value >= Factor) {
				*ptr++ = '0' + interval;
			}
		} else if constexpr (MaxDigits == 2) {
			if (MinDigits >= 2 || value >= 10 * Factor) {
				*ptr++ = decimal_lut[interval * 2];
			}
			if (MinDigits >= 1 || value >= Factor) {
				*ptr++ = decimal_lut[interval * 2 + 1];
			}
		} else {
			auto compute = [] {
				int low_digits = 1;
				T coeff = 10;
				while (low_digits * 2 < MaxDigits) {
					low_digits *= 2;
					coeff *= coeff;
				}
				return std::make_pair(low_digits, coeff);
			};
			constexpr auto computed = compute();
			constexpr int low_digits = computed.first;
			constexpr T coeff = computed.second;
			write_int_split<T, (T)(Factor * coeff), std::max(0, MinDigits - low_digits), MaxDigits - low_digits>(value, interval / coeff);
			write_int_split<T, Factor, std::min(MinDigits, low_digits), low_digits>(value, interval % coeff);
		}
	}

	template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	fastio_ostream& operator<<(const T& value) {
		if constexpr (std::is_integral_v<T>) {
			std::make_unsigned_t<T> abs = value;
			if (value < 0) {
				*ptr++ = '-';
				abs = -abs;
			}
			constexpr int decimal_lengths[] = {3, 5, 10, 20};
			constexpr int length = decimal_lengths[__builtin_ctz(sizeof(value))];
			write_int_split<decltype(abs), 1, 1, length>(abs, abs);
		} else {
			T abs = value;
			if (value < 0) {
				*ptr++ = '-';
				abs = -abs;
			}
			if (abs == 0) {
				return *this << '0';
			}
			if (abs >= 1e16) {
				abs *= 1e-16;
				int exp = 16;
				while (abs >= 1) {
					abs *= 0.1;
					exp++;
				}
				*ptr++ = '0';
				*ptr++ = '.';
				unsigned int n = abs * 1e8;
				write_int_split<unsigned int, 1, 8, 8>(n, n);
				*ptr++ = 'e';
				*this << exp;
			} else if (abs >= 1) {
				unsigned long long whole = abs;
				*this << whole;
				if (abs != whole) {
					*ptr++ = '.';
					unsigned int n = (abs - whole) * 1e8;
					write_int_split<unsigned int, 1, 8, 8>(n, n);
				}
			} else {
				*ptr++ = '0';
				*ptr++ = '.';
				unsigned int n = abs * 1e8;
				write_int_split<unsigned int, 1, 8, 8>(n, n);
			}
		}
		return *this;
	}

	fastio_ostream& operator<<(const char* const& value) {
		// We'd prefer strcpy without null terminator here, but perhaps strcpy itself suffices. It's
		// also a builtin in GCC, which means outputting a constant string is going to be optimized
		// into a mov or two!
		ptr = (NonAliasingChar*)stpcpy((char*)ptr, value);
		return *this;
	}
	fastio_ostream& operator<<(const unsigned char* const& value) {
		return *this << (const char*)value;
	}
	fastio_ostream& operator<<(const signed char* const& value) {
		return *this << (const char*)value;
	}

	fastio_ostream& operator<<(const std::string& value) {
		std::memcpy(ptr, value.data(), value.size());
		ptr += value.size();
		return *this;
	}
	fastio_ostream& operator<<(const std::string_view& value) {
		std::memcpy(ptr, value.data(), value.size());
		ptr += value.size();
		return *this;
	}

	template<typename T>
	fastio_ostream& operator<<(const std::complex<T>& value) {
		return *this << '(' << ' ' << value.real() << ',' << ' ' << value.imag() << ')';
	}

	template<size_t N>
	SIMD fastio_ostream& operator<<(const std::bitset<N>& value) {
		size_t i = N;
#ifdef AVX2
		while (i % 32 > 0) {
			*ptr++ = '0' + value[--i];
		}
		char* p = (char*)ptr;
		while (i >= 32) {
			_mm256_storeu_si256(
				(__m256i*)p,
				_mm256_sub_epi8(
					_mm256_set1_epi8('0'),
					_mm256_cmpeq_epi8(
						_mm256_and_si256(
							_mm256_shuffle_epi8(
								_mm256_set1_epi32(((uint32_t*)&value)[(i -= 32) / 32]),
								_mm256_set_epi64x(0x0000000000000000, 0x0101010101010101, 0x0202020202020202, 0x0303030303030303)
							),
							_mm256_set1_epi64x(0x0102040810204080)
						),
						_mm256_set1_epi64x(0x0102040810204080)
					)
				)
			);
			p += 32;
		}
#else
		while (i % 16 > 0) {
			*ptr++ = '0' + value[--i];
		}
		char* p = (char*)ptr;
		while (i >= 16) {
			_mm_storeu_si128(
				(__m128i*)p,
				_mm_sub_epi8(
					_mm_set1_epi8('0'),
					_mm_cmpeq_epi8(
						_mm_and_si128(
							_mm_shuffle_epi8(
								_mm_set1_epi16(((uint16_t*)&value)[(i -= 16) / 16]),
								_mm_set_epi64x(0x0000000000000000, 0x0101010101010101)
							),
							_mm_set1_epi64x(0x0102040810204080)
						),
						_mm_set1_epi64x(0x0102040810204080)
					)
				)
			);
			p += 16;
		}
#endif
		ptr = (NonAliasingChar*)p;
		return *this;
	}

	fastio_ostream& operator<<(fastio_ostream& (*func)(fastio_ostream&)) {
		return func(*this);
	}
};

struct fastio_ignoreostream {
	template<typename T>
	fastio_ignoreostream& operator<<(const T& value) {
		return *this;
	}
	fastio_ignoreostream& operator<<(fastio_ignoreostream& (*func)(fastio_ignoreostream&)) {
		return func(*this);
	}
};

}

namespace std {
	fastio::fastio_istream fastio_cin(0);
	fastio::fastio_ostream fastio_cout(1);
	fastio::fastio_ignoreostream fastio_cerr;

	fastio::fastio_istream& getline(fastio::fastio_istream& in, std::string& value, char delim = '\n') {
		if (*in.ptr == '\0') {
			in.is_ok = false;
			return in;
		}
		auto start = in.ptr;
		in.trace_line();
		// We know there's no overlap, so avoid doing this for a little bit of performance:
		// value.assign((const char*)start, in.ptr - start);
		((std::basic_string<fastio::UninitChar>&)value).resize(in.ptr - start);
		std::memcpy(value.data(), (const char*)start, in.ptr - start);
		in.ptr += *in.ptr == '\r';
		in.ptr++;
		return in;
	}

	fastio::fastio_ostream& endl(fastio::fastio_ostream& stream) {
		return stream << '\n';
	}
	fastio::fastio_ostream& flush(fastio::fastio_ostream& stream) {
		return stream;
	}

	fastio::fastio_ignoreostream& endl(fastio::fastio_ignoreostream& stream) {
		return stream << '\n';
	}
	fastio::fastio_ignoreostream& flush(fastio::fastio_ignoreostream& stream) {
		return stream;
	}
}

#define cin fastio_cin
#define cout fastio_cout

#ifdef ONLINE_JUDGE
#define cerr fastio_cerr
#endif

struct init {
	init() {
		struct sigaction act;
		act.sa_sigaction = on_sigbus;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGBUS, &act, NULL);
	}

	static void on_sigbus(int, siginfo_t* info, void*) {
		if (info->si_addr == std::cin.base && std::cin.file_size == -1) {
			std::cin.init();
		} else if (info->si_addr == std::cin.base + ((std::cin.file_size + 4095) & ~4095)) {
			std::cin.on_eof();
		} else if ((uintptr_t)info->si_addr - (uintptr_t)(std::cout.base + std::cout.file_size) < 4096) {
			if (std::cout.file_size == -1) {
				std::cout.init();
			} else {
				std::cout.on_eof();
			}
		} else {
			_exit(1);
		}
	}
} fastio_init;
