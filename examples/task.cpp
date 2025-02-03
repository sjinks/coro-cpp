#include <iostream>

#include "eager_task.h"
#include "task.h"

namespace {

//! [sample tasks]
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
//! [sample tasks]

}  // namespace

int main()
{
    wwa::coro::run_awaitable(print);

    // Expected output:
    // The result is 579

    return 0;
}
