#include <gtest/gtest.h>

#include <array>
#include <functional>
#include <stdexcept>

#include "async_generator.h"
#include "eager_task.h"
#include "exceptions.h"

using namespace wwa::coro;

namespace {

template<typename T>
async_generator<T> async_value(T value)
{
    co_yield value;
}

}  // namespace

TEST(AsyncGeneratorTest, DefaultConstructedIsEmpty)
{
    []() -> eager_task {
        async_generator<int> g;
        auto it = co_await g.begin();
        EXPECT_EQ(it, g.end());
    }();
}

TEST(AsyncGeneratorTest, MoveConstruct)
{
    []() -> eager_task {
        auto f = async_value(1983);
        auto g(std::move(f));

        auto f_it = co_await f.begin();  // NOLINT(bugprone-use-after-move)
        auto g_it = co_await g.begin();

        EXPECT_EQ(f_it, f.end());
        EXPECT_NE(g_it, g.end());
    }();
}

TEST(AsyncGeneratorTest, MoveAssign)
{
    []() -> eager_task {
        constexpr int expected = 1983;

        auto f = async_value(expected);
        auto g = async_value(1973);

        g = std::move(f);

        auto f_it = co_await f.begin();  // NOLINT(bugprone-use-after-move)
        auto g_it = co_await g.begin();

        EXPECT_EQ(f_it, f.end());
        EXPECT_NE(g_it, g.end());
        EXPECT_EQ(*g_it, expected);
    }();
}

TEST(AsyncGeneratorTest, MoveAssignToDefault)
{
    []() -> eager_task {
        constexpr int expected = 1973;

        auto f = async_value(expected);
        async_generator<int> g;

        g = std::move(f);

        auto f_it = co_await f.begin();  // NOLINT(bugprone-use-after-move)
        auto g_it = co_await g.begin();

        EXPECT_EQ(f_it, f.end());
        EXPECT_NE(g_it, g.end());
        EXPECT_EQ(*g_it, expected);
    }();
}

TEST(AsyncGeneratorTest, MoveAssignSelf)
{
    []() -> eager_task {
        constexpr int expected = 2002;

        auto f = async_value(expected);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
        f = std::move(f);
#pragma clang diagnostic pop

        auto it = co_await f.begin();

        EXPECT_NE(it, f.end());
        EXPECT_EQ(*it, expected);
    }();
}

TEST(AsyncGeneratorTest, DoesNotStartWithoutBegin)
{
    bool started = false;

    auto create_generator = [](std::reference_wrapper<bool> gen_started) -> async_generator<int> {
        gen_started.get() = true;
        co_yield 1983;
    };

    {
        auto g = create_generator(started);
        EXPECT_FALSE(started);
    }

    EXPECT_FALSE(started);
}

TEST(AsyncGeneratorTest, NoValues)
{
    []() -> eager_task {
        auto gen = []() -> async_generator<int> { co_return; }();

        auto it = co_await gen.begin();
        EXPECT_EQ(it, gen.end());
    }();
}

TEST(AsyncGeneratorTest, OneValue)
{
    []() -> eager_task {
        constexpr int expected = 1983;

        auto&& gen = async_value(expected);

        auto it = co_await gen.begin();

        EXPECT_NE(it, gen.end());
        EXPECT_EQ(*it, expected);

        co_await ++it;
        EXPECT_EQ(it, gen.end());
    }();
}

TEST(AsyncGeneratorTest, MultipleValues)
{
    []() -> eager_task {
        using value_type = int;
        constexpr std::array<value_type, 3U> expected{1, 2, 3};

        const auto make_gen = [](std::reference_wrapper<decltype(expected)> arr) -> async_generator<value_type> {
            for (auto value : arr.get()) {
                co_yield value;
            }
        };

        auto&& gen = make_gen(expected);

        auto it = co_await gen.begin();
        for (const auto& expected_value : expected) {
            EXPECT_NE(it, gen.end());
            EXPECT_EQ(*it, expected_value);
            co_await ++it;
        }

        EXPECT_EQ(it, gen.end());
    }();
}

TEST(AsyncGeneratorTest, ExceptionBeforeYield)
{
    auto g = []() -> async_generator<int> {
        throw std::runtime_error("Goodbye");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
        co_yield 1;
#pragma clang diagnostic pop
    }();

    [](async_generator<int> gen) -> eager_task {
        EXPECT_THROW(co_await gen.begin(), std::runtime_error);
    }(std::move(g));
}

TEST(AsyncGeneratorTest, ExceptionAfterYield)
{
    auto g = []() -> async_generator<int> {
        co_yield 1;
        throw std::runtime_error("Goodbye");
    }();

    [](async_generator<int> gen) -> eager_task {
        auto it = co_await gen.begin();
        EXPECT_EQ(*it, 1);
        EXPECT_THROW(co_await ++it, std::runtime_error);
        EXPECT_EQ(it, gen.end());
    }(std::move(g));
}

TEST(AsyncGeneratorTest, AllThingsEnd)
{
    auto g = []() -> async_generator<int> {
        co_yield 1;
        co_yield 2;
        co_yield 3;
        co_yield 4;
        co_yield 5;
    }();

    [](async_generator<int> gen) -> eager_task {
        while (co_await gen.begin() != gen.end()) {
            // Do nothing
        }

        EXPECT_EQ(co_await gen.begin(), gen.end());
        EXPECT_EQ(co_await gen.begin(), gen.end());
    }(std::move(g));
}

TEST(AsyncGeneratorIteratorTest, AccessEndIterator)
{
    []() -> eager_task {
        auto gen = async_value(1983);

        auto it = co_await gen.begin();
        EXPECT_NE(it, gen.end());
        co_await ++it;
        EXPECT_EQ(it, gen.end());
        EXPECT_THROW(*it, bad_result_access);
    }();
}

TEST(AsyncGeneratorIteratorTest, IteratePastEnd)
{
    []() -> eager_task {
        auto gen = async_value(1983);

        auto it = co_await gen.begin();
        EXPECT_NE(it, gen.end());
        co_await ++it;
        EXPECT_EQ(it, gen.end());

        EXPECT_THROW(co_await ++it, bad_result_access);
        EXPECT_EQ(it, gen.end());
    }();
}

TEST(AsyncGeneratorIteratorTest, CompareIterators)
{
    auto g = []() -> async_generator<int> {
        co_yield 1;
        co_yield 2;
    }();

    [](async_generator<int> gen) -> eager_task {
        auto it1 = co_await gen.begin();
        auto it2 = it1;
        auto end = gen.end();

        EXPECT_EQ(it1, it1);
        EXPECT_EQ(it1, it2);

        co_await ++it1;
        EXPECT_EQ(it1, it2);

        co_await ++it2;
        EXPECT_EQ(it1, it2);

        EXPECT_EQ(it1, end);
        EXPECT_EQ(it2, end);

        auto it3 = co_await gen.begin();
        EXPECT_EQ(it3, end);
        EXPECT_EQ(it1, it3);
        EXPECT_EQ(it3, it2);

        EXPECT_EQ(co_await gen.begin(), co_await gen.begin());
    }(std::move(g));
}

TEST(AsyncGeneratorIteratorTest, CompareDifferentIterators)
{
    auto f = []() -> async_generator<int> {
        co_yield 1;
        co_yield 2;
    }();

    async_generator<int> g;

    [](async_generator<int> g1, async_generator<int> g2) -> eager_task {
        EXPECT_EQ(co_await g2.begin(), g2.end());
        EXPECT_NE(co_await g1.begin(), co_await g2.begin());
        EXPECT_NE(co_await g2.begin(), co_await g1.begin());
        EXPECT_EQ(co_await g1.begin(), co_await g2.begin());
        EXPECT_EQ(co_await g2.begin(), co_await g1.begin());
    }(std::move(f), std::move(g));
}

TEST(AsyncGeneratorIteratorTest, CompareIteratorsAfterFinish)
{
    auto g = []() -> async_generator<int> {
        co_yield 1;
        co_yield 2;
    }();

    [](async_generator<int> gen) -> eager_task {
        auto it1 = co_await gen.begin();
        auto it2 = it1;

        co_await ++it1;
        EXPECT_NE(it1, gen.end());
        EXPECT_NE(it2, gen.end());

        co_await ++it1;
        EXPECT_EQ(it1, gen.end());
        // it2 == end() because its coroutine is done
        EXPECT_EQ(it2, gen.end());
    }(std::move(g));
}
