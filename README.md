# WWA Coro — Yet Another C++20 Coroutine Library

[![Build and Test](https://github.com/sjinks/coro-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/sjinks/coro-cpp/actions/workflows/ci.yml)
[![CodeQL](https://github.com/sjinks/coro-cpp/actions/workflows/codeql.yml/badge.svg)](https://github.com/sjinks/coro-cpp/actions/workflows/codeql.yml)

This project provides a set of coroutine-based asynchronous utilities and generators for C++20. It includes synchronous and asynchronous generators, tasks, and various utilities to facilitate coroutine-based programming.

## Features

- **Synchronous Generators**: Create coroutine-based generators that produce values synchronously.
- **Asynchronous Generators**: Create coroutine-based generators that produce values asynchronously.
- **Tasks**: Manage coroutine-based tasks that produce values of a specified type.

## Getting Started

### Prerequisites

- C++20 compatible compiler
- CMake 3.23 or higher

### Installation

```sh
cmake -B build
cmake --build build
sudo cmake --install build
```

#### Configuration Options

| Option                  | Description                                                               | Default |
|-------------------------|---------------------------------------------------------------------------|---------|
| `BUILD_TESTS`           | Build tests                                                               | `ON`    |
| `BUILD_EXAMPLES`        | Build examples                                                            | `ON`    |
| `BUILD_DOCS`            | Build documentation                                                       | `ON`    |
| `BUILD_INTERNAL_DOCS`   | Build internal documentation                                              | `OFF`   |
| `ENABLE_MAINTAINER_MODE`| Maintainer mode (enable more compiler warnings, treat warnings as errors) | `OFF`   |
| `USE_CLANG_TIDY`        | Use `clang-tidy` during build                                             | `OFF`   |

The `BUILD_DOCS` (public API documentation) and `BUILD_INTERNAL_DOCS` (public and private API documentation) require [Doxygen](https://www.doxygen.nl/)
and, optionally, `dot` (a part of [Graphviz](https://graphviz.org/)).

The `USE_CLANG_TIDY` option requires [`clang-tidy`](https://clang.llvm.org/extra/clang-tidy/).

#### Build Types

| Build Type       | Description                                                                     |
|------------------|---------------------------------------------------------------------------------|
| `Debug`          | Build with debugging information and no optimization.                           |
| `Release`        | Build with optimization for maximum performance and no debugging information.   |
| `RelWithDebInfo` | Build with optimization and include debugging information.                      |
| `MinSizeRel`     | Build with optimization for minimum size.                                       |
| `ASAN`           | Build with AddressSanitizer enabled for detecting memory errors.                |
| `LSAN`           | Build with LeakSanitizer enabled for detecting memory leaks.                    |
| `UBSAN`          | Build with UndefinedBehaviorSanitizer enabled for detecting undefined behavior. |
| `TSAN`           | Build with ThreadSanitizer enabled for detecting data races.                    |
| `Coverage`       | Build with code coverage analysis enabled.                                      |

ASAN, LSAN, UBSAN, TSAN, and Coverage builds are only supported with GCC or clang.

Coverage build requires `gcov` (GCC) or `llvm-gcov` (clang) and [`gcovr`](https://gcovr.com/en/stable/).

### Running Tests

To run the tests, use the following command:

```sh
ctest -T test --test-dir build/test
```

or run the following binary:

```sh
./build/test/coro_test
```

The test binary uses [Google Test](http://google.github.io/googletest/) library.
Its behavior [can be controlled](http://google.github.io/googletest/advanced.html#running-test-programs-advanced-options)
via environment variables and/or command line flags.

Run `coro_test --help` for the list of available options.

## Usage Examples

The documentation is available at [https://sjinks.github.io/coro-cpp/](https://sjinks.github.io/coro-cpp/).

### Eager Coroutine (`eager_task`)

An *eager coroutine* start execution immediately upon creation and does not suspend upon completion. From the caller's perspective,
it runs synchoronously; the control is returned to the caller only after the coroutine finishes.

```cpp
#include <wwa/coro/eager_task.h>

wwa::coro::eager_task my_task()
{
    co_await some_other_coroutine();
}
```

See [examples/eager_task.cpp](https://github.com/sjinks/coro-cpp/blob/master/examples/eager_task.cpp)

Eager coroutines can `co_await` other coroutines but they cannot be `co_await`'ed.

### Task (`task<T>`)

*Tasks* are lightweight coroutines that start executing when they are awaited; they can optionally return values (the type of the return value
is determined by the template parameter `T`). Use tasks to create your coroutines, and use `co_await` or `co_yield` within tasks
to perform asynchronous operations.

```cpp
#include <wwa/coro/task.h>

wwa::coro::task<int> task1()
{
    co_return 123;
}

wwa::coro::task<int> task2()
{
    co_return 456;
}

wwa::coro::task<int> sum()
{
    const auto a = co_await task1();
    const auto b = co_await task2();
    co_return a + b;
}

wwa::coro::task<> print()
{
    std::cout << "The result is " << co_await sum() << "\n";
}
```

It is possible to turn any task (or any awaitable) into a fire-anf-forget eager coroutine. For the example above,

```cpp
#include <wwa/coro/eager_task.h>
#include <wwa/coro/task.h>

wwa::coro::run_awaitable(print);
```

See [examples/task.cpp](https://github.com/sjinks/coro-cpp/blob/master/examples/task.cpp).

### Generators (`generator<T>`)

*Generators* are special coroutines that produce sequences of values of a specified type (`T`). These values are produced lazily and *synchronously*.

The coroutine body is able to yield values of type `T` using the `co_yield` keyword.
However, the coroutine body is not able to use the `co_await` keyword; values **must** be produced synchronously.

Generators can be used with range-based `for` loops and ranges.

```cpp
#include <wwa/coro/generator.h>

wwa::coro::generator<int> fibonacci(int n)
{
    int a = 0;
    int b = 1;

    if (n > 0) {
        co_yield a;
    }

    if (n > 1) {
        co_yield b;
    }

    for (int i = 2; i < n; ++i) {
        auto s = a + b;
        co_yield s;
        a = b;
        b = s;
    }
}

// ...

std::cout << "The first 10 Fibonacci numbers are: ";
for (auto n : fibonacci(10)) {
    std::cout << n << ' ';
}
```

See [examples/generator.cpp](https://github.com/sjinks/coro-cpp/blob/master/examples/generator.cpp).

### Asynchronous Generators (`async_generator<T>`)

*Generators* are special coroutines that produce sequences of values of a specified type (`T`). These values are produced lazily and *asynchronously*.

Unlike the synchronous counterpart, the coroutine body is able to use both `co_await` and `co_yield` expressions.

```cpp
// Simulates an asynchronous task
wwa::coro::task<int> get_next_value(int n)
{
    co_return n + 1;
}

wwa::coro::async_generator<int> async_first_n(int n)
{
    int v = 0;
    while (v < n) {
        co_yield v;
        v = co_await get_next_value(v);
    }
}

// ...

auto gen = async_first_n(5);
auto it  = co_await gen.begin();  // IMPORTANT! co_await is required
auto end = gen.end();

while (it != end) {
    std::cout << *it << "\n";
    co_await ++it;                // IMPORTANT! co_await is required
}
```

See [examples/async_generator.cpp](https://github.com/sjinks/coro-cpp/blob/master/examples/async_generator.cpp).

Unfortunately, it is impossible to use asynchronous iterators directly in range-based `for` loops.
However, this limitation can be overcome with the help of eager coroutines.

There is a helper adapter, `sync_generator_adapter<T>`, that turns an asynchronous generator into a quasi-synchronous one:

```cpp
#include <wwa/coro/async_generator.h>
#include <wwa/coro/sync_generator_adapter.h>

wwa::coro::async_generator<int> async_iota(int start = 0)
{
    for (int i = start;; ++i) {
        co_yield i;
    }
}

// ...

wwa::coro::sync_generator_adapter<int> sync_iota(async_iota(10));

for (auto& n : sync_iota | std::views::take(5)) {
    std::cout << n << "\n";
}
```

See [examples/sync_generator_adapter.cpp](https://github.com/sjinks/coro-cpp/blob/master/examples/sync_generator_adapter.cpp).
