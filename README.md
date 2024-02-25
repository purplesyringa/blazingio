# blazingio

![Tests](https://github.com/purplesyringa/blazingio/actions/workflows/test.yml/badge.svg)

**blazingio** is a performant minimal drop-in replacement for C++ standard input and output facilities. It's stupid, doesn't guard against invalid data and can't do much, but it can read/write integers, strings, and other primitives 5x faster than libstdc++ with `sync_with_stdio(false);`, and that's what matters in areas like competitive programming.

![Aggregated benchmark](aggregated-benchmark.svg)


## How do I use this in produciton?

You don't. For the love of god, don't use this library anywhere but for programming competitions. **IT CAN TRIGGER ANY SORT OF UB ON INVALID INPUT, INCLUDING SECURITY VULNERABILITIES**.


## I'm really just a Codeforces participant, how do I use this?

Copy-paste [blazingio.min.hpp](blazingio.min.hpp) from this repository into your working program, right after all your includes. You're all set now.

Yes, really. The selling point is you don't have to re-learn I/O. It's just magically optimized.


## `.min.hpp`? What's that JSism doing in my C++?

Certain programming competitions impose a limit on the source code size, often 64 KiB or 256 KiB. Regardless of how minimal the library is by C++ standards, anything larger than a few kilobytes in source code is likely to hinder its use by the competitive programming community.

Additionally, participants may be expected to read each other's code during a hack session, and while it's typically useful to be able to read algorithms themselves, such meager things as optimized I/O are just clutter. Indeed, blazingio can handle anything [testlib](https://github.com/MikeMirzayanov/testlib) accepts and is covered with tests, so it can be thought of as "obviously correct" if you're looking for a logic error, just like libstdc++.

So here's a compromise: I compress the library so that it's so small and tidy there are few downsides in copy-pasting it straight to your template. The compressed version also includes a link to this repository so that the code is not considered obfuscated.


## What are the limitations?

blazingio's mocked `std::cin` can read most types `std::iostream` can via `operator>>`:

- `char`
- non-`char` integral types, e.g. `int`
- `float`, `double`
- `std::complex<T>`
- `std::string`, `std::string_view`, `const char*`, etc.
- `std::bitset<N>`

Other methods are not supported. `std::getline` is available, though.

The differences in format are:

- Integral types can't start with positive sign `+`.
- Floating-point accuracy is insignificantly worse than 1 ulp.
- Only decimal formats are supported for both ints and floats.
- `std::bitset<N>` can't handle input strings shorter than `N` (std would zero-pads them).
- Parameters of PRNGs (`std::linear_congruential_engine` and `std::uniform_int_distribution`) cannot be loaded. (Did you even know these *can* be read?)
- Pointers (`const void*`) cannot be read. (There's no compelling reason to.)

Failures are not handled in any way, i.e. they may cause UB. The only handled condition is EOF, which you can check via `if (std::cin)`, just like with std. You're supposed to stop reading input the moment this happens: if you keep reading, you'll trigger UB.

Similar considerations apply to `std::cout`. Most types can be written, although the exact format may differ from the one produced by std:

- Floating-point numbers less than 1e16 are formatted with exactly 12 digits after the decimal separator: `0.100000000000`, `0.012300000000`, `0.333333333333`, `123456789123456.125000000000`. Floating-point numbers greater than 1e16 are formatted in scientific-like notation: `0.123456789012e17`. This notably differs from std, which uses scientific notation for numbers less than 1 too, but this use case is rather uncommon.
- Parameters of PRNGs (`std::linear_congruential_engine` and `std::uniform_int_distribution`) cannot be formatted.
- Pointers (`const void*` and `std::nullptr_t`) cannot be written. (There's no compelling reason to.)
- `precision` and `width` are not handled.

It also goes without saying that you won't be able to use this library if you can't or aren't allowed to use external code.


## Other features

With `-DONLINE_JUDGE` (enabled on Codeforces, among others), `std::cerr` is replaced with a mock that doesn't do anything. This means you don't have to comment out logs for performance.

`freopen` works with blazingio's streams, provided that you call `freopen` before any other I/O.

Here's a neat bonus: you can make your own slim build of blazingio, stripped of anything you don't need and thus much smaller, by editing the configuration file at `config` and running `python3 minimize.py`.
