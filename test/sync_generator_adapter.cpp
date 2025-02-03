#include <gtest/gtest.h>

#include <numeric>
#include "async_generator.h"
#include "sync_generator_adapter.h"
#include "task.h"

using namespace wwa::coro;

namespace {

template<typename T>
task<T> get_async_value(T n)
{
    co_return n;
}

template<typename T>
requires(std::is_arithmetic_v<T>)
async_generator<T> async_fibonacci(T n)
{
    T a(0);
    T b(1);

    if (n > 0) {
        co_yield co_await get_async_value<T>(a);
    }

    if (n > 1) {
        co_yield co_await get_async_value<T>(b);
    }

    for (T i = 2; i < n; ++i) {
        auto s = a + b;
        co_yield co_await get_async_value<T>(s);
        a = b;
        b = s;
    }
}

template<typename T>
requires(std::is_arithmetic_v<T>)
async_generator<T> async_first_n(T n)
{
    T v{0};
    while (v < n) {
        co_yield co_await get_async_value<T>(v++);
    }
}

template<typename T>
requires(std::is_arithmetic_v<T>)
async_generator<T> async_iota()
{
    T v{1};
    while (true) {
        co_yield co_await get_async_value<T>(v++);
    }
}

}  // namespace

TEST(SyncGeneratorAdapterTest, iota)
{
    using value_type = unsigned int;

    sync_generator_adapter<value_type> gen(async_iota<value_type>());

    constexpr value_type expected_count = 5U;
    constexpr value_type expected_sum   = 15U;

    value_type actual_count = 0U;
    value_type actual_sum   = 0U;
    for (auto&& n : gen | std::views::take(5)) {
        ++actual_count;
        actual_sum += n;
    }

    EXPECT_EQ(actual_count, expected_count);
    EXPECT_EQ(actual_sum, expected_sum);
}

TEST(SyncGeneratorAdapterTest, Fibonacci)
{
    using value_type           = unsigned int;
    constexpr value_type limit = 10;

    constexpr std::array<value_type, limit> expected{0, 1, 1, 2, 3, 5, 8, 13, 21, 34};
    std::array<value_type, expected.size()> actual{};

    sync_generator_adapter<value_type> gen(async_fibonacci<value_type>(expected.size()));

    for (decltype(actual)::size_type idx = 0; auto i : gen) {
        actual.at(idx) = i;
        ++idx;
    }

    EXPECT_EQ(actual, expected);
}

TEST(SyncGeneratorAdapterTest, Sum)
{
    using value_type           = unsigned int;
    constexpr value_type count = 10U;

    auto&& generator = sync_generator_adapter<value_type>(async_first_n<value_type>(count + 1));  // 0..10

    constexpr value_type expected = 55U;
    const value_type actual       = std::accumulate(generator.begin(), generator.end(), 0U);

    EXPECT_EQ(actual, expected);
}

TEST(SyncGeneratorAdapterTest, SumIterator)
{
    using value_type           = unsigned int;
    constexpr value_type count = 10U;

    auto&& generator = sync_generator_adapter<value_type>(async_first_n(count + 1));

    constexpr value_type expected = 55U;
    value_type actual             = 0U;
    // NOLINTNEXTLINE(modernize-loop-convert) -- we want to test begin()/end()
    for (auto it = generator.begin(); it != generator.end(); ++it) {
        actual += *it;
    }

    EXPECT_EQ(actual, expected);
}

TEST(SyncGeneratorAdapterTest, AllThingsEnd)
{
    auto&& generator = sync_generator_adapter<int>(async_first_n(5));

    while (generator.begin() != generator.end()) {
    }

    EXPECT_EQ(generator.begin(), generator.end());
    EXPECT_EQ(generator.begin(), generator.end());
}
