#ifndef C6A6302D_1533_460C_84B8_A465FEABCAF6
#define C6A6302D_1533_460C_84B8_A465FEABCAF6

/**
 * @file sync_generator_adapter.h
 * @brief Adapter for converting asynchronous generators to synchronous generators.
 *
 * This header file defines the `sync_generator_adapter` class, which adapts an `async_generator` to provide a synchronous
 * generator interface. This allows asynchronous generators to be used in contexts where synchronous iteration is required.
 *
 * Example:
 * @snippet sync_generator_adapter.cpp async generator
 * @snippet{trimleft} sync_generator_adapter.cpp usage
 */

#include <cstddef>
#include <iterator>
#include <ranges>
#include <utility>

#include "async_generator.h"
#include "eager_task.h"

namespace wwa::coro {

/**
 * @brief Adapter for converting asynchronous generators to synchronous generators.
 *
 * The `sync_generator_adapter` class adapts an `async_generator` to provide a synchronous generator interface.
 * This allows asynchronous generators to be used in contexts where synchronous iteration is required.
 *
 * Example:
 * @snippet sync_generator_adapter.cpp async generator
 * @snippet{trimleft} sync_generator_adapter.cpp usage
 *
 * @tparam Result The type of the elements produced by the generator.
 */
template<typename Result>
class [[nodiscard]] sync_generator_adapter : public std::ranges::view_interface<sync_generator_adapter<Result>> {
public:
    /**
     * @brief Iterator for the synchronous generator adapter.
     *
     * The `iterator` struct provides an [input iterator](https://en.cppreference.com/w/cpp/iterator/input_iterator)
     * interface for the synchronous generator adapter.
     */
    struct iterator {
        /** @brief The strongest iterator concept supported by the iterator. */
        using iterator_concept = std::input_iterator_tag;

        /**
         * @brief Difference between the addresses of any two elements in the controlled sequence
         * @note This is required to satisfy the `std::ranges::view` concept. It does not really make sense for generators.
         */
        using difference_type = std::ptrdiff_t;

        /// @cond INTERNAL
        /**
         * @brief Default constructor.
         *
         * Constructs a sentinel iterator that does not point to any element.
         */
        iterator() noexcept : m_op(nullptr) {}

        /**
         * @brief Constructs an iterator from an `async_generator` advance operation awaitable.
         *
         * @param op The advance operation awaitable.
         */
        explicit iterator(const async_generator<Result>::iterator::advance_op& op) : m_op(std::move(op))
        {
            this->advance();
        }

        /**
         * @brief Destructor.
         */
        ~iterator() = default;
        /// @endcond

        /**
         * @brief Default copy constructor
         *
         * Constructs an iterator by copying the contents of another iterator.
         *
         * @param other The other iterator to copy.
         * @note This constructor is required to satisfy the `std::ranges::view` concept.
         */
        iterator(const iterator& other) noexcept = default;

        /**
         * @brief Default move constructor.
         *
         * Constructs an iterator by moving the contents of another iterator.
         *
         * @param other The other iterator to move.
         */
        iterator(iterator&& other) noexcept = default;

        /**
         * @brief Default copy assignment operator.
         *
         * Assigns the contents of another iterator to this iterator.
         *
         * @param other The other iterator to copy.
         * @return Reference to this iterator.
         * @note This operator is required to satisfy the `std::ranges::view` concept.
         */
        iterator& operator=(const iterator& other) noexcept = default;

        /**
         * @brief Default move assignment operator.
         *
         * Assigns the contents of another iterator to this iterator by moving them.
         *
         * @param other The other iterator to move.
         * @return Reference to this iterator.
         */
        iterator& operator=(iterator&& other) noexcept = default;

        /**
         * @brief Advances the iterator to the next element.
         *
         * @return A reference to the advanced iterator.
         */
        iterator& operator++() { return this->advance(); }

        /**
         * @brief Advances the iterator to the next element.
         *
         * @note This operator is required to satisfy the `std::ranges::view` concept.
         */
        void operator++(int) { this->operator++(); }

        /**
         * @brief Dereferences the iterator to access the current element.
         *
         * @return A reference to the current element.
         */
        async_generator<Result>::iterator::reference operator*() const { return *this->m_it; }

        /**
         * @brief Compares two iterators for equality.
         *
         * @param other The other iterator to compare with.
         * @return `true` if the iterators are equal, `false` otherwise.
         */
        bool operator==(const iterator& other) const noexcept { return this->m_it == other.m_it; }

    private:
        async_generator<Result>::iterator::advance_op m_op;  ///< The advance operation awaitable.
        async_generator<Result>::iterator m_it{nullptr};     ///< The asynchronous generator iterator.

        /**
         * @brief Advances the iterator to the next element.
         *
         * This method synchronously advances the asynchronous iterator to the next element.
         *
         * @return A reference to self.
         */
        iterator& advance()
        {
            [](iterator* self) -> eager_task { self->m_it = co_await self->m_op; }(this);
            return *this;
        }
    };

    /**
     * @brief Class constructor.
     *
     * Constructs a synchronous generator adapter from an asynchronous generator.
     *
     * @param gen The asynchronous generator to adapt.
     */
    explicit sync_generator_adapter(async_generator<Result> gen) noexcept : m_gen(std::move(gen)) {}

    /**
     * @brief Move constructor.
     *
     * Constructs a synchronous generator adapter by moving the contents of another adapter.
     *
     * @param other The other adapter to move.
     */
    sync_generator_adapter(sync_generator_adapter&& other) noexcept = default;

    /**
     * @brief Destructor.
     */
    ~sync_generator_adapter() = default;

    /// @cond
    sync_generator_adapter(const sync_generator_adapter&)            = delete;
    sync_generator_adapter& operator=(const sync_generator_adapter&) = delete;
    sync_generator_adapter& operator=(sync_generator_adapter&&)      = delete;
    /// @endcond

    /**
     * @brief Returns an iterator to the **current** item of the generator.
     *
     * This method returns an iterator to the current item of the generator. It is called `begin()` only to allow the generator
     * to be used in range-based for loops or with ranges. Because generators cannot be restarted, this method can be used
     * to iterate over the generator:
     * @snippet{trimleft} advance_with_begin.cpp Iterate over adapted asynchronous generator with begin()
     *
     * @return Iterator to the beginning of the generator.
     */
    iterator begin() { return iterator(this->m_gen.begin()); }

    /**
     * @brief Returns a sentinel iterator that marks the end of the generator.
     *
     * This method returns a sentinel iterator that marks the end of the generator. An attempt to dereference this iterator
     * or iterate past it will result in an exception being thrown.
     *
     * @return Sentinel iterator marking the end of the generator.
     */
    iterator end() noexcept { return {}; }

private:
    async_generator<Result> m_gen;  ///< The asynchronous generator being adapted.
};

/**
 * @example sync_generator_adapter.cpp
 * Example of adapting an `async_generator` to use with `std::ranges`.
 */

/**
 * @brief Deduction guide for `sync_generator_adapter`.
 *
 * This deduction guide allows the `sync_generator_adapter` class to be constructed without explicitly specifying the template argument.
 *
 * @tparam Result The type of the elements produced by the generator.
 */
template<typename Result>
sync_generator_adapter(async_generator<Result>) -> sync_generator_adapter<Result>;

}  // namespace wwa::coro

#endif /* C6A6302D_1533_460C_84B8_A465FEABCAF6 */
