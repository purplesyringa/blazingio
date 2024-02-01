# blazingio

**blazingio** is a performant minimal drop-in replacement for C++ standard input and output facilities. It's stupid, doesn't guard against invalid data and can't do much, but it can read/write integers, strings, and other primitives 10x faster than libstdc++, and that's what matters in areas like competitive programming.


## Cut straight to the chase, how do I use it?

Copy-paste `blazingio.min.hpp` from this repository into your working program, right after all your includes. You're all set now.

Yes, really. The selling point is you don't have to re-learn I/O. It's just magically optimized.


## Back up, back up. Just how good is this?

Here's a simple program in C++ to sum up numbers:

```cpp
#include <iostream>

int main() {
	int n;
	std::cin >> n;

	long long sum = 0;
	for (int i = 0; i < n; i++) {
		int number;
		std::cin >> number;
		sum += number;
	}

	std::cout << sum << std::endl;
	return 0;
}
```

Let's benchmark it:

```shell
$ echo 1000000 >input
$ shuf -i 0-1000000000 -n 1000000 >>input
$ g++ test.cpp -O2
$ time ./a.out <input
500274260791507

real	0m0,373s
user	0m0,368s
sys	0m0,005s
```

Now add just this one line after `#include <iostream>`:

```cpp
#include "blazingio.min.hpp"
```

Let's try it out again:

```shell
$ g++ test.cpp -O2
$ time ./a.out <input
500274260791507

real	0m0,020s
user	0m0,020s
sys	0m0,001s
```

That's much better! And -- mind you -- it's *still* much better even if we try the common tricks to speed up std:

```cpp
#include <iostream>

int main() {
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(nullptr);

	int n;
	std::cin >> n;

	long long sum = 0;
	for (int i = 0; i < n; i++) {
		int number;
		std::cin >> number;
		sum += number;
	}

	std::cout << sum << std::endl;
	return 0;
}
```

```shell
$ g++ test.cpp -O2
$ time ./a.out <input
500274260791507

real	0m0,110s
user	0m0,108s
sys	0m0,001s
```

And it's still better than sprintf, while (arguably) easier to use:

```cpp
#include <cstdio>

int main() {
	int n;
	scanf("%d", &n);

	long long sum = 0;
	for (int i = 0; i < n; i++) {
		int number;
		scanf("%d", &number);
		sum += number;
	}

	printf("%lld\n", sum);
	return 0;
}
```

```shell
$ g++ test.cpp -O2
test.cpp: In function ‘int main()’:
test.cpp:5:14: warning: ignoring return value of ‘int scanf(const char*, ...)’ declared with attribute ‘warn_unused_result’ [-Wunused-result]
    5 |         scanf("%d", &n);
      |         ~~~~~^~~~~~~~~~
test.cpp:10:22: warning: ignoring return value of ‘int scanf(const char*, ...)’ declared with attribute ‘warn_unused_result’ [-Wunused-result]
   10 |                 scanf("%d", &number);
      |                 ~~~~~^~~~~~~~~~~~~~~
$ time ./a.out <input
500274260791507

real	0m0,155s
user	0m0,150s
sys	0m0,005s
```

Moreover, it's even faster than a typical custom `getc`-based parser!


## `.min.hpp`? What's that JSism doing in my C++?

Certain programming competitions impose a limit on the source code size, often 64 KiB or 256 KiB. Regardless of how minimal the library is by C++ standards, anything larger than a few kilobytes in source code is likely to hinder its use by the competitive programming community.

Additionally, participants may be expected to read each other's code during a hack session, and while it's typically useful to be able to read algorithms themselves, such meager things as optimized I/O are just clutter. Indeed, blazingio can handle anything [testlib](https://github.com/MikeMirzayanov/testlib) accepts, so it can be thought of as "obviously correct" if you're looking for a logic error, just like libstdc++.

So here's a compromise: I compress the library so that it's so small and tidy there are no downsides in copy-pasting it straight to your template. The compressed version also includes a link to this repository so that the code is not considered obfuscated.


## What are the limitations?

blazingio's mocked `std::cin` can read most types `std::iostream` can via `operator>>`, but other methods are not supported. `std::getline` is available, though.

The differences in format are:

- Integral types can't start with positive sign `+`.
- Floating-point accuracy is insignificantly worse than 1 ulp.
- Only decimal formats are supported for both ints and floats.
- `std::bitset<N>` can't handle input strings shorter than `N` (std would zero-pads them).
- Parameters of PRNGs (`std::linear_congruential_engine` and `std::uniform_int_distribution`) cannot be loaded. (Did you even know these *can* be read?)
- Pointers (`const void*`) cannot be read. (There's no compelling reason to.)

Failures are not handled in any way, i.e. they may cause UB. The only handled condition is EOF, which you can check via `if (std::cin)`, just like with std. You're supposed to stop reading input the moment this happens: if you keep reading, you'll trigger UB.

Similar considerations apply to `std::cout`. Most types can be written, although the exact format may differ from the one produced by std:

- Floating-point numbers less than 1e16 are formatted with exactly 8 digits after the decimal separator: `0.10000000`, `0.01230000`, `0.33333333`, `123456789123456.12500000`. Floating-point numbers greater than 1e16 are formatted in scientific-like notation: `0.12345678e17`. This notably differs from std, which uses scientific notation for numbers less than 1 too, but this use case is rather uncommon.
- Parameters of PRNGs (`std::linear_congruential_engine` and `std::uniform_int_distribution`) cannot be formatted.
- Pointers (`const void*` and `std::nullptr_t`) cannot be written. (There's no compelling reason to.)
- `precision` and `width` are not handled.

It also goes without saying that you won't be able to use this library if you can't or aren't allowed to use external code.

Finally, this library overrides SIGBUS handler in some cases (namely, if `stdin_eof` or `late_binding` are set in config), so if you rely on that signal, you'll need to patch blazingio.


## How do I use this in produciton?

You don't. For the love of god, don't use this anywhere but for programming competitions. If "**IT CAN TRIGGER ANY SORT OF UB ON INVALID INPUT, INCLUDING SECURITY VULNERABILITIES**" was not obvious enough, maybe consider that it overrides signal handlers by design.


## Other features

With `-DONLINE_JUDGE` (enabled on Codeforces, among others), `std::cerr` is replaced with a mock that doesn't do anything. This means you don't have to comment out logs for performance.

`freopen` works with blazingio's streams, provided that you call `freopen` before any other I/O.

Here's a neat bonus: you can make your own slim build of blazingio, stripped of anything you don't need and thus much smaller, by editing the configuration file at `config` and running `python3 minimize.py`. You might need to patch the script by replacing the path to clang-format, though.
