#include <iostream>

#include "eager_task.h"
#include "task.h"

namespace {

//! [eager_task example]
// Sample awaitable
wwa::coro::task<int> my_task()
{
    co_return 3;
}

wwa::coro::eager_task my_eager_task()
{
    const auto result = co_await my_task();  // Can be any awaitable
    std::cout << "Result: " << result << "\n";
}
//! [eager_task example]

}  // namespace

int main()
{
    //! [eager_task usage]
    std::cout << "Starting eager task..." << "\n";
    my_eager_task();
    std::cout << "Eager task finished." << "\n";
    //! [eager_task usage]

    // Expected output:
    // Starting eager task...
    // Result: 3
    // Eager task finished.

    return 0;
}
