#ifndef C16AFA99_7780_4436_8BC0_60F24EF771A4
#define C16AFA99_7780_4436_8BC0_60F24EF771A4

/**
 * @file async_generator.h
 * @brief Asynchronous generator.
 *
 * This file contains the definition of an asynchronous generator class template.
 * The `async_generator` template class allows for the creation of coroutine-based generators
 * that produce values of a specified type asynchronously. It provides an interface
 * for iterating over the generated values using co_await and asynchronous iteration.
 *
 * Example usage:
 * @snippet async_generator.cpp asynchronous generator example
 * @snippet{trimleft} async_generator.cpp asynchronous generator usage
 */

#include <coroutine>
#include <exception>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "detail.h"
#include "exceptions.h"

namespace wwa::coro {

/**
 * @brief An asynchronous generator that produces values of type `Result`.
 *
 * The generator is a coroutine that produces values of type `Result` asynchronously.
 * Unlike synchronous counterparts, an asynchronous generator can use `co_await` in its body.
 * The caller must use `co_await` on the `begin()` iterator and its increment operator.
 *
 * Example:
 * @snippet async_generator.cpp asynchronous generator example
 * @snippet{trimleft} async_generator.cpp asynchronous generator usage
 *
 * @tparam Result The type of the values produced by the generator.
 */
template<typename Result>
class [[nodiscard]] async_generator {
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
        /** The type of the values produced by the generator with all references removed. */
        using value_type = std::remove_reference_t<Result>;
        /** Type of the pointer to the current value. */
        using pointer    = std::add_pointer_t<value_type>;

        /**
         * @brief Default constructor.
         *
         * Constructs a new promise object.
         */
        promise_type() noexcept = default;

        /**
         * @brief Destructor.
         */
        ~promise_type() = default;

        /// @cond
        promise_type(const promise_type&)            = delete;
        promise_type(promise_type&&) noexcept        = delete;
        promise_type& operator=(const promise_type&) = delete;
        promise_type& operator=(promise_type&&)      = delete;
        /// @endcond

        /**
         * @brief Creates a new generator object from the promise.
         *
         * This method creates a new generator object from the promise.
         *
         * @return An instance of `async_generator`.
         */
        auto get_return_object() noexcept { return async_generator<Result>{*this}; }

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
        auto final_suspend() noexcept
        {
            this->m_current_value = nullptr;
            return yield_op{this->m_consumer};
        }

        /**
         * @brief Processes exceptions that leaked from the coroutine's body.
         *
         * This method is called if an exception is thrown from the coroutine's body and not caught.
         * It saves the thrown exception so that it can be rethrown later.
         *
         * @see `rethrow_if_unhandled_exception()`
         */
        void unhandled_exception() noexcept { this->m_exception = std::current_exception(); }

        /**
         * @brief Handles the exit out of the coroutine body.
         *
         * This method is called when the coroutine returns. Does nothing.
         */
        constexpr void return_void() const noexcept {}

        /**
         * @brief Yields a value from the generator.
         *
         * This method yields an awaitable value from the generator.
         *
         * @param value The value to yield (as an lvalue reference).
         * @return An awaitable yield operation.
         *
         * @internal
         * @test @a AsyncGeneratorTest.OneValue etc
         */
        auto yield_value(value_type& value) noexcept
        {
            this->m_current_value = std::addressof(value);
            return yield_op{this->m_consumer};
        }

        /**
         * @brief Yields a value from the generator.
         *
         * This method yields an awaitable value from the generator.
         *
         * @param value The value to yield (as an rvalue reference).
         * @return An awaitable yield operation.
         *
         * @internal
         * @test @a AsyncGeneratorTest.ExceptionAfterYield
         * @test @a AsyncGeneratorIteratorTest.AccessEndIterator
         * @test @a AsyncGeneratorIteratorTest.IteratePastEnd
         */
        auto yield_value(std::add_rvalue_reference_t<std::remove_cv_t<value_type>> value) noexcept
        {
            this->m_current_value = std::addressof(value);
            return yield_op{this->m_consumer};
        }

        /**
         * @brief Retrieves the current value of the generator.
         *
         * This method retrieves the current value of the generator.
         *
         * @return An lvalue reference to the current value.
         */
        [[nodiscard]] constexpr std::add_lvalue_reference_t<value_type> value() const noexcept
        {
            return *this->m_current_value;
        }

        /**
         * @brief Checks whether the yielded value is available.
         *
         * @return @a true if the yielded value is available, @a false otherwise.
         */
        [[nodiscard]] constexpr bool finished() const noexcept { return this->m_current_value == nullptr; }

        /**
         * @brief Rethrows the unhandled exception.
         *
         * This method rethrows the unhandled exception that was saved by `unhandled_exception()`.
         *
         * @throw std::exception_ptr The unhandled exception.
         * @test @a AsyncGeneratorTest.ExceptionBeforeYield: generator rethrows an exception occurred before the first `yield`.
         * @test @a AsyncGeneratorTest.ExceptionAfterYield: generator rethrows an exception occurred after the first `yield`.
         */
        void rethrow_if_unhandled_exception()
        {
            if (this->m_exception) {
                std::rethrow_exception(std::move(this->m_exception));
            }
        }

        /**
         * @brief Sets the consumer coroutine.
         *
         * This method sets the consumer coroutine. The yield operation awaitable transfers control to it at the suspend point.
         * The consumer coroutine is the one that advances the generator iterator.
         *
         * @param consumer The consumer coroutine.
         */
        void set_consumer(std::coroutine_handle<> consumer) noexcept { this->m_consumer = consumer; }

    private:
        /** @brief Pointer to the current value of the generator; `nullptr` if the value is not yet set. */
        pointer m_current_value = nullptr;
        /** @brief The consumer coroutine. */
        std::coroutine_handle<> m_consumer;
        /** @brief An unhandled exception, if any. */
        std::exception_ptr m_exception = nullptr;

        // NOLINTBEGIN(readability-convert-member-functions-to-static)
        /**
         * @brief An awaitable for the yield operation.
         *
         * This class is used to represent the yield operation in the generator.
         * The awaitable transfers control to the consumer coroutine at the suspend point.
         */
        class yield_op {
        public:
            /**
             * @brief Constructs a new yield operation.
             * @param consumer The consumer coroutine.
             */
            constexpr explicit yield_op(std::coroutine_handle<> consumer) noexcept : m_consumer(consumer) {}

            /**
             * @brief Determines whether the coroutine should suspend or continue immediately.
             *
             * This method always returns `false` to force coroutine suspension and transfer control to the consumer coroutine.
             *
             * @return `false` to force coroutine suspension.
             */
            [[nodiscard]] constexpr bool await_ready() const noexcept { return false; }

            /**
             * @brief Handles suspension of the coroutine and schedules its resumption.
             *
             * This method suspends the coroutine and transfers control to the consumer coroutine.
             *
             * @param h The coroutine handle of the awaiting coroutine.
             * @return The consumer coroutine.
             */
            [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] std::coroutine_handle<> h) const noexcept
            {
                return this->m_consumer;
            }

            /**
             * @brief Retrieves the result of the awaited operation once the coroutine resumes.
             *
             * This method does nothing because the awaitable does not produce a result.
             */
            constexpr void await_resume() const noexcept {}

        private:
            /** @brief The consumer coroutine. */
            std::coroutine_handle<> m_consumer;
        };
        // NOLINTEND(readability-convert-member-functions-to-static)

        /// @endcond
    };

    /**
     * @brief An [input iterator](https://en.cppreference.com/w/cpp/iterator/input_iterator) that asynchronously produces values of type `Result`.
     *
     * The iterator is used to traverse the values produced by the generator.
     * The caller must use `co_await` on the `begin()` iterator and its increment operator.
     *
     * Example:
     * @snippet{trimleft} async_generator.cpp asynchronous generator usage
     */
    class iterator {
    public:
        /// @cond INTERNAL

        /**
         * @brief Awaitable object that advances the iterator to the next value.
         *
         * This class is used to advance the iterator to the next value produced by the generator.
         * The awaitable keeps track of the consumer (a coroutine that is advancing the iterator) and the producer (the generator coroutine).
         * It sets the consumer coroutine upon suspension and returns the producer coroutine for resumption. When the awaitable is resumed,
         * it returns the iterator to the current value.
         */
        class advance_op {
        public:
            /**
             * @brief Default constructor.
             *
             * Constructs a new advance operation that will return a sentinel iterator.
             *
             * @internal
             * @test @a AsyncGeneratorTest.DefaultConstructedIsEmpty: `begin()` invoked on an empty generator must return a sentinel iterator.
             */
            constexpr advance_op() noexcept : m_promise(nullptr), m_producer(nullptr) {}

            /**
             * @brief Constructs a new advance operation.
             *
             * Constructs a new advance operation that will advance the iterator to the next value.
             *
             * @param producer Producer coroutine (the generator itself).
             */
            explicit advance_op(std::coroutine_handle<promise_type> producer) noexcept
                : m_promise(std::addressof(producer.promise())), m_producer(producer)
            {}

            /**
             * @brief Determines whether the coroutine should suspend or continue immediately.
             *
             * This method checks if the generator is empty and does not suspend if it is.
             *
             * @return Whether the coroutine should continue.
             * @retval true The coroutine should continue, as the result is available immediately (sentinel iterator)
             * @retval false The coroutine should suspend.
             *
             * @internal
             * @test @a AsyncGeneratorTest.DefaultConstructedIsEmpty: when `begin()` is invoked on an empty generator, the awaitable should not suspend.
             */
            [[nodiscard]] constexpr bool await_ready() const noexcept { return this->m_promise == nullptr; }

            /**
             * @brief Handles suspension of the coroutine and schedules its resumption.
             * 
             * This method suspends the coroutine, records the awaiting coroutine, and transfers control to the producer coroutine.
             *
             * @param consumer The coroutine handle of the awaiting coroutine (consumer).
             * @return The producer coroutine.
             */
            constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<> consumer) noexcept
            {
                this->m_promise->set_consumer(consumer);
                return this->m_producer;
            }

            /**
             * @brief Retrieves the result of the awaited operation once the coroutine resumes.
             *
             * This method returns an iterator to the current value.
             *   * If the generator is empty, it returns a sentinel iterator.
             *   * If the generator has finished without yielding anything, it rethrows the unhandled exception (if any)
             * and returns a sentinel iterator.
             *   * Otherwise, it returns an iterator to the current value.
             *
             * @return The iterator to the current value or a sentinel iterator.
             * @internal
             * @test @a AsyncGeneratorTest.DefaultConstructedIsEmpty: when `begin()` is invoked on an empty generator, a sentinel iterator must be returned.
             * @test @a AsyncGeneratorTest.NoValues: when generator finishes without yielding anything, a sentinel iterator must be returned.
             * @test @a AsyncGeneratorTest.ExceptionBeforeYield: if an exception occurred before the first `yield`, it must be rethrown.
             * @test @a AsyncGeneratorTest.OneValue and others: an iterator to the current value must be returned.
             */
            iterator await_resume()
            {
                if (this->m_promise == nullptr) {
                    return iterator{nullptr};
                }

                if (this->m_promise->finished()) {
                    this->m_promise->rethrow_if_unhandled_exception();
                    return iterator{nullptr};
                }

                return iterator{std::coroutine_handle<promise_type>::from_promise(*this->m_promise)};
            }

        private:
            /** @brief Pointer to the coroutine promise; `nullptr` for an empty generator. */
            promise_type* m_promise;
            /** @brief The producer coroutine. */
            std::coroutine_handle<> m_producer;
        };

        /// @endcond

        /** @brief The strongest iterator concept supported by the iterator. */
        using iterator_concept = std::input_iterator_tag;

        /**
         * @brief Difference between the addresses of any two elements in the controlled sequence.
         * @note This is required to satisfy the `std::ranges::view` concept. It does not really make sense for generators.
         */
        using difference_type = std::ptrdiff_t;

        /// @cond INTERNAL
        /** @brief Value type; equivalent to generator's result type with all references stripped. */
        using value_type = std::remove_reference_t<Result>;
        /** @brief Reference type; an lvalue reference to the @a value_type. */
        using reference  = std::add_lvalue_reference_t<value_type>;
        /** @brief Pointer type; used to store a pointer to the current generator value. */
        using pointer    = std::add_pointer_t<value_type>;

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
         * @brief Advances the iterator to the next value.
         *
         * This method advances the iterator to the next value produced by the generator.
         * 
         * @attention The caller must use `co_await` on the return value of this operator.
         *
         * @return Awaitable object that advances the iterator to the next value.
         */
        [[nodiscard]] advance_op operator++()
        {
            if (detail::is_good_handle(this->m_coroutine)) {
                return advance_op{this->m_coroutine};
            }

            /** @internal @test @a AsyncGeneratorIteratorTest.IteratePastEnd */
            throw bad_result_access{"Incrementing past the end of the generator"};
        }

        /**
         * @brief Dereferences the iterator to obtain the current value.
         *
         * This method dereferences the iterator to obtain the current value produced by the generator.
         *
         * @return An lvalue reference to the current value produced by the generator.
         * @throw bad_result_access Attempting to dereference the end of the generator.
         */
        reference operator*() const
        {
            if (detail::is_good_handle(this->m_coroutine)) [[likely]] {
                return this->m_coroutine.promise().value();
            }

            /** @internal @test @a AsyncGeneratorIteratorTest.AccessEndIterator */
            throw bad_result_access{"Dereferencing the end of the generator"};
        }

        /**
         * @brief Compares the iterator @a it with the sentinel @a sentinel.
         *
         * This method compares the iterator with the sentinel for equality. If the iterator is equal to the sentinel,
         * it means that the generator is finished and is at the end.
         *
         * @param it Left hand side of the comparison.
         * @param sentinel Right hand side of the comparison.
         * @return `true` if the iterator is equal to the sentinel, `false` otherwise.
         * @internal
         * @test @a AsyncGeneratorTest.DefaultConstructedIsEmpty: `it.m_coroutine == nullptr` when `begin()` is invoked on an empty generator.
         * @test @a AsyncGeneratorTest.NoValues: `it.m_coroutine == nullptr` when generator finished without yielding anything.
         * @test @a AsyncGeneratorTest.OneValue etc: `it.m_coroutine != nullptr` when generator yields a value.
         * @test @a AsyncGeneratorIteratorTest.CompareIteratorsAfterFinish: `it.m_coroutine.done() == true` when we have two generators, and one of them is finished.
         */
        friend bool operator==(const iterator& it, [[maybe_unused]] std::default_sentinel_t sentinel) noexcept
        {
            return !detail::is_good_handle(it.m_coroutine);
        }

        /**
         * @brief Compares two iterators for equality.
         *
         * This method compares two iterators for equality. Two iterators are equal if they share the same coroutine
         * or when their coroutines are both invalid or finished.
         *
         * @param other The other iterator to compare with.
         * @return `true` if the iterators are equal, `false` otherwise.
         * @internal
         * @test @a AsyncGeneratorIteratorTest.CompareIterators: compares iterators of the same generator.
         */
        bool operator==(const iterator& other) const noexcept
        {
            return this->m_coroutine == other.m_coroutine ||
                   (!detail::is_good_handle(this->m_coroutine) && !detail::is_good_handle(other.m_coroutine));
        }

    private:
        std::coroutine_handle<promise_type> m_coroutine;  ///< The coroutine handle.
    };

    /**
     * @brief Default constructor.
     *
     * Constructs an empty generator (`begin() == end()`).
     *
     * @internal
     * @test @a AsyncGeneratorTest.DefaultConstructedIsEmpty
     */
    constexpr async_generator() noexcept = default;

    /**
     * @brief Move constructor.
     *
     * Constructs a generator by moving the coroutine handle from another generator.
     *
     * @param other The other generator to move from.
     * @internal
     * @test @a AsyncGeneratorTest.MoveConstruct
     */
    constexpr async_generator(async_generator&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr))
    {}

    /**
     * @brief Destructor.
     *
     * Destroys the generator and the coroutine handle.
     *
     * @internal
     * @test @a AsyncGeneratorTest.DefaultConstructedIsEmpty: `this->m_coroutine == nullptr` for an empty generator.
     * @test @a AsyncGeneratorTest.DoesNotStartWithoutBegin: coroutine is destroyed without starting.
     */
    ~async_generator()
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
     * @internal
     * @test @a AsyncGeneratorTest.MoveAssignSelf: do not allow self-assignment.
     * @test @a AsyncGeneratorTest.MoveAssign: destroy old coroutine.
     * @test @a AsyncGeneratorTest.MoveAssignToDefault: do not destroy a `nullptr` coroutine.
     */
    async_generator& operator=(async_generator&& other) noexcept
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
    async_generator(const async_generator&)            = delete;
    async_generator& operator=(const async_generator&) = delete;
    /// @endcond

    /**
     * @brief Returns an awaitable iterator to the **current** item of the generator.
     *
     * This method returns an awaitable iterator to the current item of the generator.
     * Because generators cannot be restarted, this method can be used to iterate over the generator:
     * @snippet{trimleft} advance_with_begin.cpp Iterate over asynchronous generator with begin()
     *
     * @attention The caller must use `co_await` on the return value of this method.
     *
     * @return Awaitable iterator.
     * @internal
     * @test @a AsyncGeneratorTest.DefaultConstructedIsEmpty: `begin()` is invoked on an empty generator; sentinel must be returned.
     * @test @a AsyncGeneratorTest.AllThingsEnd: ensure `begin()` does not advance past end.
     * @test @a AsyncGeneratorTest.NoValues etc: operation of a non-empty generator.
     */
    [[nodiscard]] auto begin() noexcept
    {
        if (!detail::is_good_handle(this->m_coroutine)) {
            return typename iterator::advance_op();
        }

        return typename iterator::advance_op{this->m_coroutine};
    }

    /**
     * @brief Returns a sentinel iterator.
     *
     * This method returns a sentinel iterator that marks the end of the generator.
     *
     * @return Sentinel iterator.
     */
    [[nodiscard]] constexpr auto end() const noexcept { return std::default_sentinel; }

private:
    std::coroutine_handle<promise_type> m_coroutine;  ///< The coroutine handle.

    /**
     * @brief Constructs a new asynchronous generator from a promise.
     *
     * @param promise The promise object.
     */
    explicit async_generator(promise_type& promise) noexcept
        : m_coroutine(std::coroutine_handle<promise_type>::from_promise(promise))
    {}
};

/**
 * @example async_generator.cpp
 * Example of using an asynchronous generator.
 */

}  // namespace wwa::coro

#endif /* C16AFA99_7780_4436_8BC0_60F24EF771A4 */
