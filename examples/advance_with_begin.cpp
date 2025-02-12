#include <iostream>
#include <utility>

#include "async_generator.h"
#include "eager_task.h"
#include "generator.h"
#include "task.h"

namespace {

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

wwa::coro::generator<int> sync_first_n(int n)
{
    int v = 0;
    while (v < n) {
        co_yield v;
        ++v;
    }
}

wwa::coro::eager_task iterate_over_async()
{
    //! [Iterate over asynchronous generator with begin()]
    auto gen = async_first_n(5);
    auto it  = co_await gen.begin();
    auto end = gen.end();

    do {
        std::cout << *it << " ";
    } while (co_await gen.begin() != end);
    //! [Iterate over asynchronous generator with begin()]

    std::cout << "\n";
}

void iterate_over_sync()
{
    //! [Iterate over synchronous generator with begin()]
    auto gen = sync_first_n(5);
    auto it  = gen.begin();
    auto end = gen.end();

    do {
        std::cout << *it << " ";
    } while (gen.begin() != end);
    //! [Iterate over synchronous generator with begin()]

    std::cout << "\n";
}

}  // namespace

int main()
{
    std::cout << "Iterating over asynchronous generator:\n";
    iterate_over_async();
    std::cout << "Iterating over synchronous generator:\n";
    iterate_over_sync();

    // Expected output:
    // Iterating over asynchronous generator:
    // 0 1 2 3 4
    // Iterating over synchronous generator:
    // 0 1 2 3 4

    return 0;
}
