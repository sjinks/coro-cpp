#include <iostream>

#include "async_generator.h"
#include "eager_task.h"
#include "task.h"

namespace {

// Simulates an asynchronous task
wwa::coro::task<int> get_next_value(int n)
{
    co_return n + 1;
}

//! [asynchronous generator example]
wwa::coro::async_generator<int> async_first_n(int n)
{
    int v = 0;
    while (v < n) {
        co_yield v;
        // Asynchronous generators can use `co_await`; synchronous ones cannot
        v = co_await get_next_value(v);
    }
}
//! [asynchronous generator example]

wwa::coro::eager_task async_generator_example()
{
    //! [asynchronous generator usage]
    auto gen = async_first_n(5);
    auto it  = co_await gen.begin();  // IMPORTANT! co_await is required
    auto end = gen.end();

    while (it != end) {
        std::cout << *it << "\n";
        co_await ++it;  // IMPORTANT! co_await is required
    }
    //! [asynchronous generator usage]
}

}  // namespace

int main()
{
    async_generator_example();

    // Expected output:
    // 0
    // 1
    // 2
    // 3
    // 4

    return 0;
}
