#include <cstddef>
#include <iostream>

#include "generator.h"

namespace {

//! [generator example]
wwa::coro::generator<int> first_n(std::size_t n)
{
    int v = 0;
    while (static_cast<std::size_t>(v) < n) {
        co_yield v;
        ++v;
    }
}
//! [generator example]

}  // namespace

int main()
{
    std::cout << "The first 5 numbers are:\n";
    //! [range-for example]
    for (auto n : first_n(5)) {
        std::cout << n << ' ';
    }
    //! [range-for example]

    std::cout << '\n';

    //! [manual iteration example]
    auto gen = first_n(5);
    for (auto it = gen.begin(); it != gen.end(); ++it) {  // NOSONAR
        std::cout << *it << ' ';
    }
    //! [manual iteration example]

    std::cout << '\n';

    // The expected output is:
    // The first 5 numbers are:
    // 0 1 2 3 4
    // 0 1 2 3 4

    return 0;
}
