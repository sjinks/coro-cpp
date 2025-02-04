#include <iostream>
#include <ranges>

#include "async_generator.h"
#include "sync_generator_adapter.h"

namespace {

//! [async generator]
wwa::coro::async_generator<int> async_iota(int start = 0)
{
    for (int i = start;; ++i) {
        co_yield i;
    }
}
//! [async generator]

}  // namespace

int main()
{
    //! [usage]
    wwa::coro::sync_generator_adapter sync_iota(async_iota(10));

    for (const auto& n : sync_iota | std::views::take(5)) {
        std::cout << n << "\n";
    }
    //! [usage]

    // Expected output:
    // 10
    // 11
    // 12
    // 13
    // 14

    return 0;
}
