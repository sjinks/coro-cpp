#include <iostream>

#include "eager_task.h"
#include "task.h"

namespace {

wwa::coro::task<int> async_increment(int v)
{
    co_return v + 1;
}

wwa::coro::task<> display(int v)
{
    auto result = co_await async_increment(v);
    std::cout << "Result: " << result << "\n";
}

}  // namespace

int main()
{
    wwa::coro::run_awaitable(display, 2);

    // Expected output:
    // Result: 3

    return 0;
}
