#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include "eager_task.h"
#include "exceptions.h"
#include "task.h"

using namespace wwa::coro;

namespace {

template<typename T>
// NOLINTNEXTLINE(performance-unnecessary-value-param,cppcoreguidelines-avoid-reference-coroutine-parameters)
task<T> value(T what)
{
    if constexpr (std::is_rvalue_reference_v<T>) {
        co_return std::move(what);
    }
    else {
        co_return what;
    }
}

task<void> value()
{
    co_return;
}

}  // namespace

TEST(TaskTest, Basic)
{
    []() -> eager_task {
        const auto actual = co_await value(true);
        EXPECT_TRUE(actual);
    }();
}

TEST(TaskTest, DoesNotStartUntilAwaited)
{
    bool started = false;

    [](std::reference_wrapper<bool> flag) -> eager_task {
        const auto func = [](std::reference_wrapper<bool> f) -> task<> {
            f.get() = true;
            co_return;
        };

        auto coro = func(flag);
        EXPECT_FALSE(flag.get());

        co_await coro;
        EXPECT_TRUE(flag.get());
    }(started);
}

TEST(TaskTest, SynchronousCompletion)
{
#if defined(__GNUC__) && !defined(__clang__)
    GTEST_SKIP() << "This test crashes under GCC with optimizations disabled or with a sanitizer enabled";
#endif

    []() -> eager_task {
        int actual             = 0;
        constexpr int expected = 100000;
        for (int i = 0; i < expected; ++i) {
            actual += co_await value(1);
        }

        EXPECT_EQ(actual, expected);
    }();
}

TEST(TaskTest, SynchronousCompletionAlt)
{
    []() -> eager_task {
        int actual             = 0;
        constexpr int expected = 100000;
        auto term              = value(1);
        for (int i = 0; i < expected; ++i) {
            actual += co_await term;
        }

        EXPECT_EQ(actual, expected);
    }();
}

TEST(TaskTest, Destroy)
{
    auto coro = value();

    EXPECT_FALSE(coro.is_ready());

    EXPECT_TRUE(coro.destroy());
    EXPECT_TRUE(coro.is_ready());

    EXPECT_FALSE(coro.destroy());
    EXPECT_TRUE(coro.is_ready());
    EXPECT_FALSE(coro.resume());
    EXPECT_THROW(coro.result_value(), bad_task);
}

TEST(TaskTest, Exception)
{
    auto thrower = []() -> task<> {
        throw std::runtime_error("error");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
        co_return;
#pragma clang diagnostic pop
    }();

    [](task<> t) -> eager_task { EXPECT_THROW(co_await t, std::runtime_error); }(std::move(thrower));
}

TEST(TaskTest, Resume)
{
    auto hello = value<std::string>("Hello");
    auto world = value<std::string>("World");

    EXPECT_FALSE(hello.is_ready());
    EXPECT_FALSE(world.is_ready());

    hello.resume();
    world.resume();

    EXPECT_TRUE(hello.is_ready());
    EXPECT_TRUE(world.is_ready());

    EXPECT_EQ(hello.result_value(), "Hello");
    EXPECT_EQ(world.result_value(), "World");
}

TEST(TaskTest, ResultValue)
{
    auto hello = value<std::string>("Hello");

    EXPECT_THROW(hello.result_value(), bad_result_access);

    hello.resume();

    EXPECT_EQ(hello.result_value(), "Hello");
}

TEST(TaskTest, ResultValueConst)
{
    const auto hello = value<std::string>("Hello");

    EXPECT_THROW(hello.result_value(), bad_result_access);

    hello.resume();

    EXPECT_EQ(hello.result_value(), "Hello");
}

TEST(TaskTest, RValueResultValue)
{
    std::string expected = "Hello";
    auto hello           = value<std::string&&>(std::move(expected));

    EXPECT_THROW(hello.result_value(), bad_result_access);

    hello.resume();

    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_TRUE(expected.empty());
    EXPECT_EQ(hello.result_value(), "Hello");
}

TEST(TaskTest, RValueResultValueConst)
{
    std::string expected = "Hello";
    const auto hello     = value<std::string&&>(std::move(expected));

    EXPECT_THROW(hello.result_value(), bad_result_access);

    hello.resume();

    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_TRUE(expected.empty());
    EXPECT_EQ(hello.result_value(), "Hello");
}

TEST(TaskTest, ConstResultValueConst)
{
    const auto hello = value<const std::string>("Hello");

    EXPECT_THROW(hello.result_value(), bad_result_access);

    hello.resume();

    EXPECT_EQ(hello.result_value(), "Hello");
}

TEST(TaskTest, ConstRefResultValueConst)
{
    const std::string expected = "Hello";
    const auto hello           = value<const std::string&>(expected);

    EXPECT_THROW(hello.result_value(), bad_result_access);

    hello.resume();

    EXPECT_EQ(hello.result_value(), "Hello");
}

TEST(TaskTest, MovedPromiseResult)
{
    auto hello = value<std::string>("Hello");
    hello.resume();

    auto actual = std::move(hello).result_value();
    // rvalue_reference result_value() &&
    EXPECT_EQ(actual, "Hello");
    // reference result_value() &
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_TRUE(hello.result_value().empty());
}

TEST(TaskTest, VoidTask)
{
    auto coro = []() -> task<> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        co_return;
    }();

    coro.resume();
    EXPECT_TRUE(coro.is_ready());
}

TEST(TaskTest, NestedTask)
{
    auto outer_coro = []() -> task<> {
        constexpr int expected = 1983;
        const auto inner_task  = value(expected);

        const auto actual = co_await inner_task;
        EXPECT_EQ(actual, expected);
    }();

    outer_coro.resume();
    EXPECT_TRUE(outer_coro.is_ready());
}

TEST(TaskTest, NestedTaskAlt)
{
    constexpr int expected_result = 1983;

    auto outer_task = [](int expected) -> task<int> {
        const auto inner_task = value(expected);

        const auto actual = co_await inner_task;
        EXPECT_EQ(actual, expected);
        co_return actual;
    };

    [](task<int> outer, int expected) -> eager_task {
        auto result = co_await outer;
        EXPECT_EQ(result, expected);
    }(outer_task(expected_result), expected_result);
}

TEST(TaskTest, DeeplyNestedTask)
{
    int sum = 0;

    const auto task1 = [](std::reference_wrapper<int> sum1) -> task<> {
        const auto task2 = [](std::reference_wrapper<int> sum2, int term2) -> task<int> {
            const auto task3 = [](std::reference_wrapper<int> sum3, int term3) -> task<int> {
                sum3.get() += term3;
                co_return term3;
            };

            constexpr int expected_task3 = 24;
            const auto actual            = co_await task3(sum2, expected_task3);
            EXPECT_EQ(actual, expected_task3);

            sum2.get() += term2;
            co_return term2;
        };

        constexpr int expected_task2 = 1983;
        const auto actual            = co_await task2(sum1, expected_task2);
        EXPECT_EQ(actual, expected_task2);
    };

    auto coro = task1(sum);

    coro.resume();
    EXPECT_TRUE(coro.is_ready());
    EXPECT_EQ(sum, 2007);
}

TEST(TaskTest, DeeplyNestedTaskAlt)
{
    []() -> eager_task {
        int sum = 0;

        const auto task1 = [](std::reference_wrapper<int> sum1) -> task<int> {
            const auto task2 = [](std::reference_wrapper<int> sum2, int term2) -> task<int> {
                const auto task3 = [](std::reference_wrapper<int> sum3, int term3) -> task<int> {
                    sum3.get() += term3;
                    co_return term3;
                };

                constexpr int expected_task3 = 24;
                const auto actual            = co_await task3(sum2, expected_task3);
                EXPECT_EQ(actual, expected_task3);

                sum2.get() += term2;
                co_return term2;
            };

            constexpr int expected_task2 = 1983;
            const auto actual            = co_await task2(sum1, expected_task2);
            EXPECT_EQ(actual, expected_task2);

            co_return sum1;
        };

        auto result = co_await task1(sum);
        EXPECT_EQ(result, 2007);
    }();
}

TEST(TaskTest, MultiSuspend)
{
    constexpr int expected = 8;
    auto coro              = []() -> task<int> {
        co_await std::suspend_always{};  // 1
        co_await std::suspend_never{};   // 2
        co_await std::suspend_always{};  // 3
        co_await std::suspend_always{};  // 4
        co_return 8;
    }();

    coro.resume();  // initial suspend
    EXPECT_FALSE(coro.is_ready());

    coro.resume();  // (1)
    EXPECT_FALSE(coro.is_ready());

    coro.resume();  // (3), as (2) is suspend_never
    EXPECT_FALSE(coro.is_ready());

    coro.resume();  // (4)
    EXPECT_TRUE(coro.is_ready());

    EXPECT_EQ(coro.result_value(), expected);
}

TEST(TaskTest, DefaultConstruct)
{
    task<int> coro;
    EXPECT_TRUE(coro.is_ready());
    EXPECT_FALSE(coro.resume());
    EXPECT_FALSE(coro.destroy());
    EXPECT_THROW(coro.result_value(), bad_task);
}

struct no_default_ctor {
    no_default_ctor() = delete;
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    int value;
};

TEST(TaskTest, DefaultConstructedParam)
{
    const task<no_default_ctor> coro;
    EXPECT_TRUE(coro.is_ready());
}

TEST(TaskTest, MoveConstruct)
{
    static const std::string expected = "something";

    auto coro = value(expected);
    EXPECT_FALSE(coro.is_ready());

    task<std::string> moved_coro{std::move(coro)};
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_TRUE(coro.is_ready());
    EXPECT_FALSE(moved_coro.is_ready());

    moved_coro.resume();

    EXPECT_TRUE(moved_coro.is_ready());
    EXPECT_EQ(moved_coro.result_value(), expected);
}

TEST(TaskTest, MoveAssign)
{
    static const std::string expected = "something";

    auto coro = value(expected);
    EXPECT_FALSE(coro.is_ready());

    task<std::string> target;
    EXPECT_TRUE(target.is_ready());

    target = std::move(coro);
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_TRUE(coro.is_ready());
    EXPECT_FALSE(target.is_ready());

    target.resume();

    EXPECT_TRUE(target.is_ready());
    EXPECT_EQ(target.result_value(), expected);
}

TEST(TaskTest, MoveAssignOther)
{
    static const std::string expected = "something";

    auto coro = value(expected);
    EXPECT_FALSE(coro.is_ready());

    auto target = value<std::string>("unexpected");
    EXPECT_FALSE(target.is_ready());

    target = std::move(coro);
    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_TRUE(coro.is_ready());
    EXPECT_FALSE(target.is_ready());

    target.resume();

    EXPECT_TRUE(target.is_ready());
    EXPECT_EQ(target.result_value(), expected);
}

TEST(TaskTest, MoveAssignSelf)
{
    static const std::string expected = "something";

    auto coro = value(expected);
    EXPECT_FALSE(coro.is_ready());

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
    coro = std::move(coro);
#pragma clang diagnostic pop
    EXPECT_FALSE(coro.is_ready());

    coro.resume();

    EXPECT_TRUE(coro.is_ready());
    EXPECT_EQ(coro.result_value(), expected);
}

TEST(TaskTest, NothingToAwait)
{
    []() -> eager_task {
        const task<> coro;
        EXPECT_THROW(co_await coro, bad_task);
        EXPECT_THROW(co_await task<>(), bad_task);
    }();
}

TEST(TaskTest, ReturnReference)
{
    static const std::string expected = "Where will you run?";

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
    struct str {
        std::string value;  // NOLINT(misc-non-private-member-variables-in-classes)

        str()           = default;
        str(const str&) = delete;
        str(str&&)      = delete;
    };

    []() -> eager_task {
        str rv;
        rv.value = expected;

        // NOLINTBEGIN(cppcoreguidelines-avoid-reference-coroutine-parameters)
        const auto return_ref  = [](str& return_value) -> task<str&> { co_return std::ref(return_value); }(rv);
        const auto return_cref = [](const str& return_value) -> task<const str&> {
            co_return std::cref(return_value);
        }(rv);

        const auto& ref = co_await return_ref;
        EXPECT_EQ(ref.value, expected);
        EXPECT_EQ(std::addressof(rv), std::addressof(ref));
        EXPECT_EQ(std::addressof(rv), std::addressof(return_ref.result_value()));

        const auto& cref = co_await return_cref;
        EXPECT_EQ(cref.value, expected);
        EXPECT_EQ(std::addressof(rv), std::addressof(cref));
        EXPECT_EQ(std::addressof(rv), std::addressof(return_cref.result_value()));
    }();
    // NOLINTEND(cppcoreguidelines-avoid-reference-coroutine-parameters)
}

TEST(TaskTest, ReturnRValueReference)
{
    []() -> eager_task {
        static const std::string expected = "Where will you hide?";

        const auto func = [](std::string s) -> task<std::string&&> { co_return std::move(s); };

        {
            auto coro             = func(expected);
            decltype(auto) actual = co_await coro;

            EXPECT_FALSE(expected.empty());
            EXPECT_EQ(actual, expected);
            EXPECT_EQ(coro.result_value(), expected);
        }

        {
            auto coro                = func(expected);
            const std::string actual = co_await coro;

            EXPECT_FALSE(expected.empty());
            EXPECT_EQ(actual, expected);
            EXPECT_TRUE(coro.result_value().empty());
        }
    }();
}
