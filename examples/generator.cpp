#include <iostream>

#include "generator.h"

namespace {

//! [generator example]
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
//! [generator example]

}  // namespace

int main()
{
    //! [usage example]
    std::cout << "The first 10 Fibonacci numbers are: ";
    for (auto n : fibonacci(10)) {
        std::cout << n << ' ';
    }
    //! [usage example]

    std::cout << '\n';

    // The expected output is:
    // The first 10 Fibonacci numbers are: 0 1 1 2 3 5 8 13 21 34

    return 0;
}
