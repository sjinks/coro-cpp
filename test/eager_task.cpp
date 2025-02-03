#include <gtest/gtest.h>

#include <exception>
#include <functional>

#include "eager_task.h"
#include "task.h"

using namespace wwa::coro;

TEST(EagerTaskTest, Basic)
{
    bool flag = false;

    const auto func = [](std::reference_wrapper<bool> f) -> eager_task {
        f.get() = true;
        co_return;
    };

    func(flag);
    EXPECT_TRUE(flag);
}

TEST(EagerTaskTest, RunAwaitable)
{
    bool flag = false;

    const auto func = [](std::reference_wrapper<bool> f) -> task<> {
        f.get() = true;
        co_return;
    };

    run_awaitable(func, flag);

    EXPECT_TRUE(flag);
}

TEST(SimpleTaskDeathTest, UnhandledException)
{
    static const std::string message = "This was the coldest night";

    std::set_terminate([] {
        if (std::current_exception()) {
            try {
                std::rethrow_exception(std::current_exception());
            }
            catch (const std::exception& e) {
                std::cerr << "Unhandled exception: " << e.what() << "\n";
            }
        }

        std::exit(1);  // NOLINT(concurrency-mt-unsafe)
    });

    const auto func = []() -> eager_task {
        throw std::runtime_error(message);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
        co_return;
#pragma clang diagnostic pop
    };

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wswitch-default"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
    EXPECT_DEATH(func(), message);
#pragma clang diagnostic pop
}
