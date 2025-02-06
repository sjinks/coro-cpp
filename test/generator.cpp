#include <gtest/gtest.h>

#include <array>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "exceptions.h"
#include "generator.h"

using namespace wwa::coro;

namespace {

template<typename T>
requires(std::is_arithmetic_v<T>)
generator<T> fibonacci(T n)
{
    T a(0);
    T b(1);

    if (n > 0) {
        co_yield a;
    }

    if (n > 1) {
        co_yield b;
    }

    for (T i = 2; i < n; ++i) {
        auto s = a + b;
        co_yield s;
        a = b;
        b = s;
    }
}

template<typename T>
requires(std::is_arithmetic_v<T>)
generator<T> first_n(T n)
{
    T v{0};
    while (v < n) {
        co_yield v++;  // NOSONAR
    }
}

template<typename T>
requires(std::is_arithmetic_v<T>)
generator<T> iota(T n = T{0})
{
    while (true) {
        co_yield n++;  // NOSONAR
    }
}

template<typename T>
generator<T> value(T v)
{
    co_yield v;
}

/**
 * The original version was buggy because it did not handle the case where the last line did not end with a newline.
 */
generator<std::vector<std::string>> split_by_lines_and_whitespace(std::string s)
{
    std::vector<std::string> res;
    std::string_view::size_type start     = 0;
    const std::string_view::size_type end = s.size();

    while (start < end) {
        auto pos = s.find_first_of(" \n", start);

        if (pos != std::string_view::npos) {
            res.emplace_back(s.substr(start, pos - start));
            start = pos + 1;
        }
        else {
            res.emplace_back(s.substr(start));
            start = end;
        }

        if (pos == std::string_view::npos || s[pos] == '\n') {
            co_yield res;
            res.clear();
        }
    }

    if (!res.empty()) {
        co_yield res;
    }
}

template<class Range>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
generator<std::pair<std::size_t, std::ranges::range_reference_t<Range>>> enumerate(Range&& r)
{
    std::size_t i = 0;
    for (auto&& val : std::forward<Range>(r)) {
        std::pair<std::size_t, std::ranges::range_reference_t<Range>> pair{i, std::forward<decltype(val)>(val)};
        co_yield pair;
        ++i;
    }
}

class test_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}  // namespace

TEST(GeneratorTest, Fibonacci)
{
    using value_type           = unsigned int;
    constexpr value_type limit = 10;

    constexpr std::array<value_type, limit> expected{0, 1, 1, 2, 3, 5, 8, 13, 21, 34};
    std::array<value_type, expected.size()> actual{};

    for (decltype(actual)::size_type idx = 0; auto i : fibonacci<value_type>(expected.size())) {
        actual.at(idx) = i;
        ++idx;
    }

    EXPECT_EQ(actual, expected);
}

TEST(GeneratorTest, Sum)
{
    using value_type           = unsigned int;
    constexpr value_type count = 10U;

    auto&& generator = first_n<value_type>(count + 1);  // 0..10

    constexpr value_type expected = 55U;
    const value_type actual       = std::accumulate(generator.begin(), generator.end(), 0U);

    EXPECT_EQ(actual, expected);
}

TEST(GeneratorTest, AdvanceWithBegin)
{
    using value_type           = unsigned int;
    constexpr value_type count = 5U;

    auto&& generator = first_n(count);
    auto it          = generator.begin();
    value_type idx   = 0;
    while (it != generator.end()) {
        EXPECT_EQ(*it, idx);
        ++idx;
        it = generator.begin();
    }

    EXPECT_EQ(idx, count);
}

TEST(GeneratorTest, AllThingsEnd)
{
    const auto&& generator = first_n(5);
    while (generator.begin() != generator.end()) {
        static_cast<void>(generator.begin());
    }

    EXPECT_EQ(generator.begin(), generator.end());
    EXPECT_EQ(generator.begin(), generator.end());
}

TEST(GeneratorTest, MoveConstruct)
{
    auto nat = [](std::size_t n) -> generator<std::size_t> {
        while (true) {
            ++n;
            co_yield n;
        }
    }(0);

    auto other = std::move(nat);

    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_EQ(nat.begin(), nat.end());
}

TEST(GeneratorTest, MoveAssign)
{
    auto g1 = []() -> generator<int> {
        co_yield 1;
        co_yield 2;
        co_yield 3;
    }();

    auto g2 = []() -> generator<int> {
        co_yield 4;
        co_yield 5;
        co_yield 6;
    }();

    g2 = std::move(g1);

    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_EQ(g1.begin(), g1.end());
    EXPECT_EQ(*g2.begin(), 1);
}

TEST(GeneratorTest, MoveAssignEmpty)
{
    auto g1 = []() -> generator<int> {
        co_yield 1;
        co_yield 2;
        co_yield 3;
    }();

    generator<int> g2;

    g2 = std::move(g1);

    // NOLINTNEXTLINE(bugprone-use-after-move)
    EXPECT_EQ(g1.begin(), g1.end());
    EXPECT_EQ(*g2.begin(), 1);
}

TEST(GeneratorTest, MoveAssignSelf)
{
    auto g = []() -> generator<int> {
        co_yield 1;
        co_yield 2;
        co_yield 3;
    }();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
    g = std::move(g);
#pragma clang diagnostic pop

    EXPECT_EQ(*g.begin(), 1);
}

TEST(GeneratorTest, View)
{
    using value_type = unsigned int;

    static_assert(std::input_iterator<generator<value_type>::iterator>);

    static_assert(std::ranges::view<generator<value_type>>);
    static_assert(std::ranges::input_range<generator<value_type>>);
    static_assert(std::ranges::viewable_range<generator<value_type>>);

    constexpr value_type expected_count = 5U;
    constexpr value_type expected_sum   = 15U;

    value_type actual_count = 0U;
    value_type actual_sum   = 0U;
    for (const auto& n : iota<value_type>(1) | std::views::take(5)) {
        ++actual_count;
        actual_sum += n;
    }

    EXPECT_EQ(actual_count, expected_count);
    EXPECT_EQ(actual_sum, expected_sum);
}

TEST(GeneratorTest, ExceptionBeforeYield)
{
    auto gen = []() -> generator<int> {
        throw test_error("Goodbye");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
        co_yield 1;
#pragma clang diagnostic pop
    }();

    EXPECT_THROW(static_cast<void>(gen.begin()), test_error);
}

TEST(GeneratorTest, ExceptionAfterYield)
{
    auto gen = []() -> generator<int> {
        co_yield 1;
        throw test_error("Goodbye");
    }();

    auto it = gen.begin();
    EXPECT_EQ(*it, 1);
    EXPECT_THROW(++it, test_error);
    EXPECT_EQ(it, gen.end());
}

TEST(GeneratorTest, IteratorUB)
{
    auto&& generator = iota<int>(1);
    // begin() does not "rewind" the generator; it gets a new value from the iterator
    // Two begin() calls, therefore, will get the second value of the sequence
    // Because the value is shared among all iterators, *it1 == *it2
    auto it1         = generator.begin();
    auto it2         = generator.begin();

    EXPECT_EQ(*it1, 2);
    EXPECT_EQ(*it2, 2);

    // Again, the values are shared
    it1++;
    EXPECT_EQ(*it1, 3);
    EXPECT_EQ(*it2, 3);
}

TEST(GeneratorTest, iota)
{
    using value_type = int;
    value_type i{0};

    for (auto n : iota<value_type>() | std::views::take(10)) {
        EXPECT_EQ(n, i);
        ++i;
    }

    for (auto n : iota(10) | std::views::take(10)) {
        EXPECT_EQ(n, i);
        ++i;
    }
}

TEST(GeneratorTest, Pointers)
{
    // The array has static storage duration, meaning it will exist for the lifetime of the program.
    constexpr std::array<const char* const, 3> values{"หนึ่ง", "สอง", "สาม"};

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines) -- `values` will outlive the lambda(the values array is declared as constexpr and has static storage duration, meaning it will exist for the lifetime of the program).
    const auto func = [&values]() -> generator<decltype(values)::value_type> {
        for (const auto& v : values) {
            co_yield v;
        }
    };

    decltype(values)::size_type i = 0;
    for (auto&& val : func()) {
        EXPECT_STREQ(val, values.at(i));
        ++i;
    }
}

TEST(GeneratorTest, EmptyGenerator)
{
    generator<int> g;

    EXPECT_EQ(g.begin(), g.end());

    EXPECT_THROW(*g.begin(), bad_result_access);
    auto it = g.begin();
    EXPECT_THROW(++it, bad_result_access);
}

TEST(GeneratorIteratorTest, AccessEndIterator)
{
    auto g = value(1);

    auto it = g.begin();
    EXPECT_EQ(*it, 1);
    ++it;

    EXPECT_EQ(it, g.end());

    EXPECT_THROW(*it, bad_result_access);
    EXPECT_THROW(*g.end(), bad_result_access);
}

TEST(GeneratorIteratorTest, AccessPastEnd)
{
    auto g = value(1);

    auto it = g.end();
    EXPECT_THROW(++it, bad_result_access);

    it = g.begin();
    ++it;

    EXPECT_EQ(it, g.end());
    EXPECT_THROW(++it, bad_result_access);
}

TEST(GeneratorIteratorTest, YieldRValue)
{
    constexpr std::string_view expected = "test";

    auto g = value<std::string>({expected.data(), expected.size()});

    decltype(auto) value = *g.begin();
    EXPECT_EQ(value, expected);
}

TEST(GeneratorIteratorTest, ModifyValue)
{
    auto&& gen = [](int n) -> generator<int> {
        while (true) {
            co_yield n;
            ++n;
        }
    }(0);

    auto it = gen.begin();
    EXPECT_EQ(*it, 0);

    *it = 10;
    ++it;

    EXPECT_EQ(*it, 11);
}

TEST(GeneratorIteratorTest, CompareIteratorsAfterFinish)
{
    auto gen = []() -> generator<int> {
        co_yield 1;
        co_yield 2;
    }();

    auto it1 = gen.begin();
    auto it2 = it1;

    ++it1;
    EXPECT_NE(it1, gen.end());
    EXPECT_NE(it2, gen.end());

    ++it1;
    EXPECT_EQ(it1, gen.end());
    // it2 == end() because its coroutine is done
    EXPECT_EQ(it2, gen.end());
}

TEST(GeneratorIteratorTest, CompareDifferentIterators)
{
    auto g1 = value(1);
    auto g2 = value(2);

    auto it1 = g1.begin();
    auto it2 = g2.begin();

    EXPECT_NE(it1, it2);
    EXPECT_NE(it2, it1);

    while (g2.begin() != g2.end()) {
        static_cast<void>(g2.begin());
    }

    EXPECT_EQ(g2.begin(), g1.end());
    EXPECT_EQ(g2.begin(), g2.end());

    EXPECT_NE(it1, g2.begin());
    EXPECT_NE(g2.begin(), it1);
}

/**
 * @see https://github.com/TartanLlama/generator/blob/2a912502de4f97dcba4f95c958ee0ddf7bc22cf5/tests/test.cpp
 */
TEST(TLGeneratorTest, split)
{
    const auto* const input = "one two three\nfour five six\nseven eight nine";

    // clang-format off
    const std::vector<std::vector<std::string>> expected = {
        {"one", "two", "three"},
        {"four", "five", "six"},
        {"seven", "eight", "nine"}
    };
    // clang-format on

    auto&& gen = split_by_lines_and_whitespace(input);

    std::size_t cnt = 0;
    for (const auto& [i, val] : enumerate(gen)) {
        EXPECT_EQ(val, expected.at(i));
        ++cnt;
    }

    EXPECT_EQ(cnt, expected.size());
}
