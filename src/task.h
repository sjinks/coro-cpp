#ifndef B6E7FCEC_A837_4D2C_B71A_E02D65C82406
#define B6E7FCEC_A837_4D2C_B71A_E02D65C82406

/**
 * @file task.h
 * @brief Coroutine-based task.
 *
 * This file contains the definition of a coroutine-based task class template.
 * The task class allows for the creation of coroutine-based tasks that produce
 * values of a specified type. It provides an interface for handling asynchronous
 * operations and managing the state of the coroutine.
 *
 * Example:
 * @snippet task.cpp sample tasks
 */

#include <cassert>
#include <coroutine>
#include <exception>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "detail.h"
#include "exceptions.h"

/** @brief Library namespace. */
namespace wwa::coro {

template<typename Result>
class task;

/// @cond INTERNAL

namespace detail {

// NOLINTBEGIN(readability-convert-member-functions-to-static)
template<typename Promise>
struct promise_base {
    /**
     * @internal
     * @test @a TaskTest.Exception
     */
    void unhandled_exception() noexcept { this->m_exception = std::current_exception(); }

    [[nodiscard]] constexpr auto initial_suspend() const noexcept { return std::suspend_always{}; }

    [[nodiscard]] constexpr auto final_suspend() const noexcept
    {
        struct nested_awaiter {
            [[nodiscard]] constexpr bool await_ready() const noexcept { return false; }

            [[nodiscard]] auto await_suspend(std::coroutine_handle<Promise> handle) const noexcept
            {
                const auto& promise = handle.promise();
                return promise.m_next ? promise.m_next : std::noop_coroutine();
            }

            void await_resume() const noexcept {}  // GCOVR_EXCL_LINE -- this method is not used
        };

        return nested_awaiter{};
    }

    void set_next(std::coroutine_handle<> next) noexcept { this->m_next = next; }

protected:
    /**
     * @internal
     * @test @a TaskTest.Exception
     */
    void rethrow_if_exception() const
    {
        if (this->m_exception != nullptr) {
            std::rethrow_exception(this->m_exception);
        }
    }

private:
    std::coroutine_handle<> m_next;
    std::exception_ptr m_exception = nullptr;

    friend Promise;  ///< Derived classes need to access the constructor.

    /**
     * @brief Default constructor.
     * @see https://clang.llvm.org/extra/clang-tidy/checks/bugprone/crtp-constructor-accessibility.html
     */
    promise_base() noexcept = default;
};
// NOLINTEND(readability-convert-member-functions-to-static)

template<typename T>
struct promise_type : promise_base<promise_type<T>> {
    using value_type = std::remove_reference_t<T>;
    using storage_type =
        std::conditional_t<std::is_lvalue_reference_v<T>, std::add_pointer_t<value_type>, std::optional<value_type>>;
    using reference = std::conditional_t<std::is_rvalue_reference_v<T>, T, std::add_lvalue_reference_t<value_type>>;

    auto get_return_object() { return task<T>{std::coroutine_handle<promise_type>::from_promise(*this)}; }

    /**
     * @internal
     * @test @a TaskTest.ConstResultValueConst
     */
    void return_value(const T& res) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires(std::is_const_v<T> && !std::is_lvalue_reference_v<T>)
    {
        this->m_result.emplace(res);
    }

    /**
     * @internal
     * @test @a TaskTest.ResultValue
     * @test @a TaskTest.RValueResultValue
     */
    void return_value(T res) noexcept(std::is_nothrow_move_constructible_v<T>)
    requires(!std::is_const_v<T> && !std::is_lvalue_reference_v<T>)
    {
        this->m_result = std::move(res);
    }

    /**
     * @internal
     * @test @a TaskTest.ConstRefResultValueConst
     */
    void return_value(T& res) noexcept
    requires(std::is_lvalue_reference_v<T>)
    {
        this->m_result = std::addressof(res);
    }

    /**
     * @internal
     * @test @a TaskTest.RValueResultValue: not a reference + bad result access
     * @test @a TaskTest.ResultValue: rvalue reference + bad result access
     * @test @a TaskTest.ConstRefResultValueConst: lvalue reference + bad result access
     */
    reference result_value()
    {
        this->rethrow_if_exception();
        if constexpr (std::is_lvalue_reference_v<T>) {
            if (this->m_result != nullptr) {
                return *this->m_result;
            }
        }
        else if (this->m_result.has_value()) {
            if constexpr (std::is_rvalue_reference_v<T>) {
                return std::move(this->m_result.value());
            }
            else {
                return this->m_result.value();
            }
        }

        throw bad_result_access("task<T> result accessed before it was set");
    }

private:
    /**
     * @brief The result produced by the task.
     *
     * @see https://en.cppreference.com/w/cpp/language/zero_initialization
     * > If `T` is a scalar type, the object is initialized to the value obtained
     * > by explicitly converting the integer literal `0` (zero) to `T`.
     * When `storage_type` is a pointer, it is initialized to `nullptr`.
     */
    storage_type m_result{};
};

template<>
struct promise_type<void> : promise_base<promise_type<void>> {
    task<void> get_return_object();
    void return_void() const noexcept {}
    void result_value() const { this->rethrow_if_exception(); }
};

template<typename Promise>
class awaiter_base {
public:
    awaiter_base(std::coroutine_handle<Promise> coro) noexcept : m_coroutine(coro) {}

    [[nodiscard]] constexpr bool await_ready() const noexcept { return detail::is_ready(this->m_coroutine); }

    auto await_suspend(std::coroutine_handle<> awaiting) noexcept
    {
        this->m_coroutine.promise().set_next(awaiting);
        return this->m_coroutine;
    }

protected:
    auto coroutine()
    {
        detail::check_coroutine(this->m_coroutine);
        return this->m_coroutine;
    }

private:
    std::coroutine_handle<Promise> m_coroutine;
};

}  // namespace detail

/// @endcond

/**
 * @brief A coroutine task.
 *
 * The `task` class represents a coroutine task that can be used to perform asynchronous operations.
 * A task is a lightweight object that wraps a coroutine and provides a mechanism for resuming the coroutine
 * and obtaining the result of the operation.
 *
 * Example:
 * @snippet task.cpp sample tasks
 *
 * @tparam Result The type of the result produced by the task.
 */
template<typename Result = void>
class [[nodiscard]] task {
public:
    /**
     * @brief The promise type associated with the task.
     *
     * The `promise_type` alias is a type alias for the promise type associated with the task.
     */
    using promise_type = detail::promise_type<Result>;

    /**
     * @brief Default constructor.
     *
     * @internal
     * @test @a TaskTest.DefaultConstruct
     */
    task() noexcept = default;

    /// @cond
    task(const task&)            = delete;
    task& operator=(const task&) = delete;
    /// @endcond

    /**
     * @brief Move constructor.
     *
     * @internal
     * @test @a TaskTest.MoveConstruct
     */
    task(task&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}

    /**
     * @brief Move assignment operator.
     *
     * @internal
     * @test @a TaskTest.MoveAssign: do not destroy a `nullptr` coroutine.
     * @test @a TaskTest.MoveAssignOther: destroy old coroutine.
     * @test @a TaskTest.MoveAssignSelf: do not allow self-assignment.
     */
    task& operator=(task&& other) noexcept
    {
        if (this != std::addressof(other)) {
            if (this->m_coroutine) {
                this->m_coroutine.destroy();
            }

            this->m_coroutine = std::exchange(other.m_coroutine, nullptr);
        }

        return *this;
    }

    /**
     * @brief Destructor.
     */
    ~task()
    {
        if (this->m_coroutine) {
            this->m_coroutine.destroy();
        }
    }

    /**
     * @brief Checks if the task is ready.
     *
     * A task is considered ready if it has finished executing or has an empty coroutine handle associated with
     * (for example, a default-constructed task).
     *
     * A task that is ready cannot be resumed.
     *
     * @return Task state.
     * @retval true The task has finished executing; the result is available.
     * @return false The task has not yet finished executing.
     */
    [[nodiscard]] constexpr bool is_ready() const noexcept { return detail::is_ready(this->m_coroutine); }

    /**
     * @brief Resumes the task.
     *
     * Resuming a task causes the associated coroutine to be resumed. A task can only be resumed if it is not ready.
     *
     * @return Task state.
     * @retval true The task has not yet finished executing.
     * @retval false The task has finished executing; the result is available.
     *
     * @see is_ready()
     *
     * @internal
     * @test @a TaskTest.Resume
     */
    // NOLINTNEXTLINE(modernize-use-nodiscard)
    bool resume() const
    {
        if (!this->is_ready()) {
            this->m_coroutine.resume();
        }

        return !this->is_ready();
    }

    /**
     * @brief Destroys the task.
     *
     * Destroys the task by destroying the associated coroutine handle.
     *
     * @return Operation status.
     * @retval true The task was destroyed.
     * @retval false The task was already destroyed or was default-constructed.
     *
     * @internal
     * @test @a TaskTest.Destroy
     */
    bool destroy()
    {
        if (this->m_coroutine) {
            this->m_coroutine.destroy();
            this->m_coroutine = nullptr;
            return true;
        }

        return false;
    }

    /** @brief Value type; a `Result` with all references stripped */
    using value_type = std::remove_reference_t<Result>;
    /** @brief Reference type; `Result&&` if `Result` is an rvalue reference, `Result&` otherwise. */
    using reference =
        std::conditional_t<std::is_rvalue_reference_v<Result>, Result, std::add_lvalue_reference_t<value_type>>;
    /** @brief Constant version of `reference`. */
    using const_reference  = std::add_lvalue_reference_t<std::add_const_t<value_type>>;
    /** @brief Rvalue reference type. */
    using rvalue_reference = std::add_rvalue_reference_t<std::remove_cv_t<value_type>>;

    /**
     * @brief Returns the result produced by the task.
     *
     * Returns an lvalue or rvalue (if `Result` template parameter of the task is an rvalue reference) reference
     * to the result produced by the task.
     *
     * @return The result produced by the task.
     * @throw bad_task The task is empty or has been destroyed.
     * @throw bad_result_access The result is not yet available.
     *
     * @internal
     * @test @a TaskTest.DefaultConstruct: ensure that the coroutine handle is valid before attempting to access `promise()`.
     * @test @a TaskTest.RValueResultValue: return of an rvalue reference.
     * @test @a TaskTest.ResultValue: return of an lvalue reference.
     */
    reference result_value() &
    {
        detail::check_coroutine(this->m_coroutine);

        if constexpr (std::is_rvalue_reference_v<Result>) {
            return std::move(this->m_coroutine.promise().result_value());
        }
        else {
            return this->m_coroutine.promise().result_value();
        }
    }

    /**
     * @brief Returns the result produced by the task.
     *
     * Returns a constant lvalue reference to the result produced by the task.
     *
     * @return The result produced by the task.
     * @throw bad_task The task is empty or has been destroyed.
     * @throw bad_result_access The result is not yet available.
     *
     * @internal
     * @test @a TaskTest.ResultValueConst: return of a const lvalue reference
     */
    // NOLINTNEXTLINE(modernize-use-nodiscard)
    const_reference result_value() const&
    {
        detail::check_coroutine(this->m_coroutine);
        return this->m_coroutine.promise().result_value();
    }

    /**
     * @brief Returns the result produced by the task.
     *
     * Returns an rvalue reference to the result produced by the task.
     *
     * @return The result produced by the task.
     * @throw bad_task The task is empty or has been destroyed.
     * @throw bad_result_access The result is not yet available.
     *
     * @internal
     * @test @a TaskTest.MovedPromiseResult: return an rvalue reference when the task is an rvalue.
     */
    rvalue_reference result_value() &&
    {
        detail::check_coroutine(this->m_coroutine);
        return std::move(this->m_coroutine.promise().result_value());
    }

    /**
     * @brief Await the result of the task.
     *
     * This operator allows the task to be `co_await`'ed, returning the result produced by the task.
     * If the task's result type is an rvalue reference, it returns an rvalue reference to the result.
     * Otherwise, it returns an lvalue reference to the result.
     *
     * @return An awaiter that retrieves the result of the task.
     *
     * @internal
     * @test @a TaskTest.SynchronousCompletionAlt
     * @test @a TaskTest.NothingToAwait: `co_await coro;`
     */
    auto operator co_await() const& noexcept
    {
        struct awaiter : public detail::awaiter_base<promise_type> {
            decltype(auto) await_resume()
            {
                auto& promise = this->coroutine().promise();
                if constexpr (std::is_rvalue_reference_v<Result>) {
                    return std::move(promise.result_value());
                }
                else {
                    return promise.result_value();
                }
            }
        };

        return awaiter(this->m_coroutine);
    }

    /**
     * @brief Await the result of the task.
     *
     * This operator allows the task to be `co_await`'ed, returning the result produced by the task.
     * It is used when the task is an rvalue, ensuring that the result is moved.
     *
     * @internal
     * @test @a TaskTest.SynchronousCompletion
     * @test @a TaskTest.NothingToAwait: `co_await task<>();`
     */
    auto operator co_await() && noexcept
    requires(!std::is_void_v<Result>)
    {
        struct awaiter : public detail::awaiter_base<promise_type> {
            decltype(auto) await_resume()
            {
                auto coro = this->coroutine();
                return std::move(coro.promise().result_value());
            }
        };

        return awaiter(this->m_coroutine);
    }

private:
    std::coroutine_handle<promise_type> m_coroutine;  ///< The coroutine handle associated with the task.

    friend promise_type;  ///< The `promise_type` needs access to the coroutine handle.

    /**
     * @brief Constructor.
     *
     * Constructs a task with the specified coroutine handle.
     *
     * @param handle The coroutine handle.
     */
    explicit task(std::coroutine_handle<promise_type> handle) : m_coroutine(handle) {}
};

/**
 * @example task.cpp
 * Example of how to use tasks.
 */

/// @cond INTERNAL

inline task<void> detail::promise_type<void>::get_return_object()
{
    return task<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
}

/// @endcond

}  // namespace wwa::coro

#endif /* B6E7FCEC_A837_4D2C_B71A_E02D65C82406 */
