#ifndef DED82BB9_7596_4598_B34D_B60883A4775D
#define DED82BB9_7596_4598_B34D_B60883A4775D

/**
 * @file generator.h
 * @brief Synchronous generator.
 *
 * This file contains the definition of a synchronous generator class template.
 * The `generator` template class allows for the creation of coroutine-based generators
 * that produce values of a specified type. It provides an interface for
 * iterating over the generated values using range-based `for` loops or manual iteration.
 *
 * Example usage:
 * @snippet generator.cpp generator example
 * @snippet{trimleft} generator.cpp usage example
 */

#include <coroutine>
#include <exception>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>

#include "detail.h"
#include "exceptions.h"

namespace wwa::coro {

/**
 * @brief An synchronous generator that produces values of type `Result`.
 *
 * The `generator` class represents a coroutine generator that produces values of type ``.
 *
 * Example:
 * @snippet generator.cpp generator example
 * @snippet{trimleft} generator.cpp usage example
 *
 * @tparam Result The type of the values produced by the generator.
 */
template<typename Result>
class [[nodiscard]] generator : public std::ranges::view_interface<generator<Result>> {
public:
    /**
     * @brief The promise type of the generator.
     *
     * The `promise_type` structure defines the behavior of the generator, including how it handles exceptions and suspension points.
     *
     * @warning This class is internally used by the compiler and should not be used directly.
     */
    class promise_type {
        /// @cond INTERNAL
    public:
        /** @brief Value type; `Result` with all references stripped.  */
        using value_type     = std::remove_reference_t<Result>;
        /** @brief Reference type; an lvalue reference to `value_type`. */
        using reference_type = std::add_lvalue_reference_t<value_type>;
        /** @brief Pointer type; a pointer to `value_type`. */
        using pointer_type   = std::add_pointer_t<value_type>;

        /**
         * @brief Creates a new generator object from the promise.
         *
         * This method creates a new generator object from the promise.
         *
         * @return An instance of `generator`.
         */
        generator<Result> get_return_object() noexcept
        {
            using coroutine_handle = std::coroutine_handle<promise_type>;
            return generator<Result>{coroutine_handle::from_promise(*this)};
        }

        /**
         * @brief Issues an awaiter for initial suspend point.
         *
         * This method is called to obtain an awaiter for the initial suspend point of the generator coroutine.
         * By returning `std::suspend_always`, the coroutine suspends before it starts.
         *
         * @return Awaiter for initial suspend point.
         * @retval std::suspend_always The coroutine suspends before it starts.
         */
        [[nodiscard]] constexpr auto initial_suspend() const noexcept { return std::suspend_always{}; }

        /**
         * @brief Issues an awaiter for final suspend point.
         *
         * This method is called to obtain an awaiter for the final suspend point of the generator coroutine.
         * By returning `std::suspend_always`, the coroutine suspends at the end.
         *
         * @return Awaiter for final suspend point.
         * @retval std::suspend_always The coroutine suspends at the end.
         */
        [[nodiscard]] constexpr auto final_suspend() const noexcept { return std::suspend_always{}; }

        /**
         * @brief Yields a value from the generator.
         *
         * This method is called when the generator yields a value. It saves the address of the yielded value and suspends the coroutine.
         *
         * @param value The value to yield (as an lvalue reference).
         * @return Awaitable to suspend the coroutine.
         *
         * @internal
         * @test @a GeneratorTest.Fibonacci
         */
        constexpr auto yield_value(std::add_const_t<reference_type> value) noexcept
        {
            this->m_value = std::addressof(value);
            return std::suspend_always{};
        }

        /**
         * @brief Yields a value from the generator.
         *
         * This method is called when the generator yields a value. It saves the address of the yielded value and suspends the coroutine.
         *
         * @param value The value to yield (as an lvalue reference).
         * @return Awaitable to suspend the coroutine.
         *
         * @internal
         * @test @a GeneratorTest.Sum
         * @test @a GeneratorTest.SumIterator
         */
        constexpr auto yield_value(std::add_rvalue_reference_t<std::remove_cvref_t<value_type>> value) noexcept
        {
            /*
             * This seems to be safe.
             *   - `yield` calls `generator_promise::yield_value(T&&)`;
             *   - `yield_value` stores the address of the temporary value;
             *   - the generator suspends and the control is transferred to the calling function;
             *   - after the generator is resumed, the temporary gets destroyed.
             */
            this->m_value = std::addressof(value);
            return std::suspend_always{};
        }

        /**
         * @brief Processes exceptions that leaked from the coroutine's body.
         *
         * This method is called if an exception is thrown from the coroutine's body and not caught.
         * It saves the thrown exception so that it can be rethrown later.
         *
         * @see `rethrow_if_exception()`
         * @internal
         * @test @a GeneratorTest.ExceptionAfterYield: exceptions thrown after `co_yield` must be rethrown.
         * @test @a GeneratorTest.ExceptionBeforeYield: exceptions thrown before `co_yield` must be rethrown.
         *
         */
        constexpr void unhandled_exception() noexcept { this->m_exception = std::current_exception(); }

        /**
         * @brief Handles the exit out of the coroutine body.
         *
         * This method is called when the coroutine returns. Does nothing.
         */
        constexpr void return_void() const noexcept {}

        /**
         * @brief Retrieves the current value of the generator.
         *
         * This method returns an lvalue reference to the current value of the generator.
         *
         * @return An lvalue reference to the current value.
         */
        [[nodiscard]] reference_type value() const noexcept { return static_cast<reference_type>(*this->m_value); }

        /**
         * @brief Deleted to prevent use of `co_await` within this coroutine.
         *
         * Synchronous generators are not awaitable. If you need to use `co_await` in the generator, consider `asynchronous_generator`.
         */
        void await_transform() = delete;

        /**
         * @brief Rethrows an exception if one was thrown during the execution of the coroutine.
         *
         * This method rethrows the unhandled exception that was saved by `unhandled_exception()`.
         *
         * @internal
         * @test @a GeneratorTest.ExceptionAfterYield: exceptions thrown after `co_yield` must be rethrown.
         * @test @a GeneratorTest.ExceptionBeforeYield: exceptions thrown before `co_yield` must be rethrown.
         */
        void rethrow_if_exception()
        {
            if (this->m_exception) {
                std::rethrow_exception(std::move(this->m_exception));
            }
        }

    private:
        /** @brief Pointer to the current value. */
        pointer_type m_value           = nullptr;
        /** @brief Unhandled exception, if any. */
        std::exception_ptr m_exception = nullptr;
        /// @endcond
    };

    /**
     * @brief An [input iterator](https://en.cppreference.com/w/cpp/iterator/input_iterator) that produces values of type `Result`.
     *
     * The iterator is used to traverse the values produced by the generator.
     *
     * Example (range-based `for`):
     * @snippet{trimleft} generator_iterator.cpp range-for example
     *
     * Example (manual iteration):
     * @snippet{trimleft} generator_iterator.cpp manual iteration example
     */
    class iterator {
    public:
        /** @brief The strongest iterator concept supported by the iterator. */
        using iterator_concept = std::input_iterator_tag;

        /**
         * @brief Difference between the addresses of any two elements in the controlled sequence.
         * @note This is required to satisfy the `std::ranges::view` concept. It does not really make sense for generators.
         */
        using difference_type = std::ptrdiff_t;

        /// @cond INTERNAL
        /** @brief Value type. */
        using value_type = promise_type::value_type;

        /**
         * @brief Constructs a new iterator object.
         *
         * This constructor creates a new iterator object from the coroutine handle.
         *
         * @param coroutine The coroutine handle.
         */
        explicit iterator(std::coroutine_handle<promise_type> coroutine) noexcept : m_coroutine(coroutine) {}
        /// @endcond

        /**
         * @brief Default constructor.
         *
         * Constructs a sentinel iterator.
         *
         * @note This constructor is required to satisfy the `std::ranges::view` concept.
         */
        iterator() noexcept = default;

        /**
         * @brief Compares two iterators.
         *
         * Two iterators are equal if they point to the same coroutine or if they are both sentinel iterators.
         *
         * @param other Another iterator.
         * @return `true` if the iterators are equal, `false` otherwise.
         * @internal
         * @test @a GeneratorTest.Fibonacci checks the `this->m_coroutine.done()` condition to stop.
         * @test @a GeneratorIteratorTest.CompareDifferentIterators checks iterators obtained from different generators.
         */
        [[nodiscard]] constexpr bool operator==(const iterator& other) const noexcept
        {
            return this->m_coroutine == other.m_coroutine ||
                   (!detail::is_good_handle(this->m_coroutine) && !detail::is_good_handle(other.m_coroutine));
        }

        /**
         * @brief Advances the iterator to the next value.
         *
         * This method advances the iterator to the next value produced by the generator.
         *
         * @return Reference to self (advanced iterator).
         * @throw bad_result_access Attempt to advance the iterator that has reached the end of the generator.
         * @internal
         * @test @test @a GeneratorIteratorTest.AccessPastEnd: attempts to advance the iterator returned by `end()` or increment
         * an iterator past the end of the generator must result in the `bas_result_access` exception.
         */
        iterator& operator++()
        {
            if (detail::is_good_handle(this->m_coroutine)) [[likely]] {
                this->m_coroutine.resume();
                if (this->m_coroutine.done()) {
                    this->m_coroutine.promise().rethrow_if_exception();
                }

                return *this;
            }

            throw bad_result_access{"Incrementing past the end of the generator"};
        }

        /**
         * @brief Advances the iterator to the next value.
         *
         * This method advances the iterator to the next value produced by the generator.
         * It is similar to the prefix increment operator but does not return a reference to the incremented iterator.
         *
         * @note Required to satisfy the `std::ranges::view` concept.
         */
        void operator++(int) { this->operator++(); }

        /**
         * @brief Dereferences the iterator to obtain the current value.
         *
         * This method dereferences the iterator to obtain the current value produced by the generator.
         *
         * @return An lvalue reference to the current value produced by the generator.
         * @throw bad_result_access Attempting to dereference the end of the generator.
         * @internal
         * @test @a GeneratorIteratorTest.AccessEndIterator: dereferencing the `end()` iterator or dereferencing an iterator that has reached the end
         * of the generator must result in the `bad_result_access` exception.
         */
        promise_type::reference_type operator*() const
        {
            if (detail::is_good_handle(this->m_coroutine)) [[likely]] {
                return this->m_coroutine.promise().value();
            }

            throw bad_result_access("Access past the end of the generator");
        }

    private:
        /** @brief Generator coroutine. */
        std::coroutine_handle<promise_type> m_coroutine;
    };

    /**
     * @brief Default constructor.
     *
     * Constructs an empty generator. This constructor is not really useful except for testing.
     *
     * @internal
     * @test @a GeneratorTest.EmptyGenerator ensures that an empty generator can be constructed and safely used.
     */
    generator() noexcept = default;

    /**
     * @brief Move constructor.
     *
     * Constructs a generator by moving the coroutine handle from another generator.
     *
     * @param other The other generator to move from.
     */
    generator(generator&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, {})) {}

    /**
     * @brief Destructor.
     *
     * Destroys the generator and the coroutine handle.
     *
     * @internal
     * @test @a GeneratorTest.MoveConstruct: coroutine must be detroyed only if it is valid.
     */
    ~generator()
    {
        if (this->m_coroutine) {
            this->m_coroutine.destroy();
        }
    }

    /**
     * @brief Move assignment operator.
     *
     * Assigns the contents of another generator to this generator by moving them.
     *
     * @param other The other generator to move.
     * @return Reference to this generator.
     *
     * @internal
     * @test @a GeneratorTest.MoveAssign: destroy the existing coroutine and move the other generator's coroutine.
     * @test @a GeneratorTest.MoveAssignEmpty: do not destroy an invalid coroutine.
     * @test @a GeneratorTest.MoveAssignSelf: disallow assignment to self.
     */
    generator& operator=(generator&& other) noexcept
    {
        if (this != std::addressof(other)) {
            if (this->m_coroutine) {
                this->m_coroutine.destroy();
            }

            this->m_coroutine = std::exchange(other.m_coroutine, nullptr);
        }

        return *this;
    }

    /// @cond
    generator(const generator&)            = delete;
    generator& operator=(const generator&) = delete;
    /// @endcond

    /**
     * @brief Returns an iterator to the **current** item of the generator.
     *
     * This method returns an iterator to the current item of the generator. It is called `begin()`
     * only to allow the generator to be used in range-based for loops or with ranges.
     *
     * Because generators cannot be restarted, this method can be used to iterate over the generator:
     * @snippet{trimleft} advance_with_begin.cpp Iterate over synchronous generator with begin()
     *
     * @return Iterator to the current item.
     * @internal
     * @test @a GeneratorTest.Fibonacci
     * @test @a GeneratorTest.AdvanceWithBegin shows how to abuse `begin()` to advance the iterator.
     */
    [[nodiscard]] iterator begin()
    {
        this->advance();
        return iterator(this->m_coroutine);
    }

    /**
     * @brief Returns a constant iterator to the **current** item of the generator.
     *
     * @see begin()
     *
     * @return Constant iterator to the current item.
     * @internal
     * @test @a GeneratorTest.SumIterator
     * @test @a GeneratorTest.View
     */
    [[nodiscard]] iterator begin() const
    {
        this->advance();
        return iterator(this->m_coroutine);
    }

    /**
     * @brief Returns a sentinel iterator that marks the end of the generator.
     *
     * This method returns a sentinel iterator that marks the end of the generator. An attempt to dereference this iterator
     * or iterate past it will result in an exception being thrown.
     *
     * @internal
     * @test @a GeneratorTest.Fibonacci
     * @test @a GeneratorTest.SumIterator
     * @test @a GeneratorTest.View
     */
    [[nodiscard]] constexpr iterator end() const noexcept { return iterator(nullptr); }

private:
    /** @brief Coroutine handle. */
    std::coroutine_handle<promise_type> m_coroutine;

    /**
     * @brief Constructs a generator from a coroutine handle.
     *
     * @param coroutine The coroutine handle.
     */
    explicit generator(std::coroutine_handle<promise_type> coroutine) noexcept : m_coroutine(coroutine) {}

    /**
     * @brief Resumes the generator.
     *
     * This method resumes the generator (if it has not yet finished) to produce the next value. Rethrows any exceptions
     * that were thrown during the execution of the coroutine.
     *
     * @internal
     * @test @a GeneratorTest.AllThingsEnd: do not call `resume()` if the coroutine is done.
     * @test @a GeneratorTest.ExceptionBeforeYield: rethrow exceptions thrown in the generator.
     */
    void advance() const
    {
        if (detail::is_good_handle(this->m_coroutine)) {
            this->m_coroutine.resume();
            if (this->m_coroutine.done()) {
                this->m_coroutine.promise().rethrow_if_exception();
            }
        }
    }
};

/**
 * @example generator.cpp
 * Example of using a generator to generate Fibonacci numbers.
 */

/**
 * @example generator_iterator.cpp
 * Example of using generator iterators with range `for` and mnaually.
 */

/**
 * @example advance_with_begin.cpp
 * Shows how to (ab)use `begin()` to advance the iterator.
 */

}  // namespace wwa::coro

#endif /* DED82BB9_7596_4598_B34D_B60883A4775D */
