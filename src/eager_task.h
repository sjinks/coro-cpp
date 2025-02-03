#ifndef CF9BADEE_8060_4165_91B2_54C840903F88
#define CF9BADEE_8060_4165_91B2_54C840903F88

/**
 * @file eager_task.h
 * @brief Eager coroutine.
 *
 * This header file defines the `eager_task` class, which represents an eager coroutine that starts execution immediately upon creation.
 * It also includes the `run_awaitable` function template, which turns any awaitable into an eager fire-and-forget coroutine.
 *
 * @snippet eager_task.cpp eager_task example
 * @snippet{trimleft} eager_task.cpp eager_task usage
 */

#include <coroutine>
#include <exception>
#include <functional>

namespace wwa::coro {

// NOLINTBEGIN(readability-convert-member-functions-to-static)
/**
 * @brief Eager coroutine.
 *
 * This class represents an eager coroutine, which starts execution immediately upon creation and does not suspend upon completion.
 * From the caller's perspective, it runs synchoronously; the control is returned to the caller only after the coroutine finishes.
 *
 * Example:
 * @snippet eager_task.cpp eager_task example
 * @snippet{trimleft} eager_task.cpp eager_task usage
 */
class eager_task {
public:
    /**
     * @brief The promise type for the eager_task coroutine.
     *
     * This structure defines the behavior of the coroutine, including how it handles return values,
     * exceptions, and suspension points.
     *
     * @warning This class is internally used by the compiler and should not be used directly.
     */
    struct promise_type {
        /// @cond INTERNAL
        /**
         * @brief Issues the coroutine object.
         *
         * This method is called to obtain the coroutine object when the coroutine is created.
         *
         * @return Coroutine (instance of `eager_task`).
         * @internal
         * @test @a SimpleTaskTest.Basic
         */
        [[nodiscard]] constexpr auto get_return_object() const { return eager_task{}; }

        /**
         * @brief Processes exceptions that leaked from the coroutine's body.
         *
         * This method is called if an exception is thrown from the coroutine's body and not caught.
         * It terminates the program on an unhandled exception.
         *
         * @internal
         * @test @a SimpleTaskDeathTest.UnhandledException ensures that the program terminates on an unhandled exception.
         */
        [[noreturn]] void unhandled_exception() noexcept { std::terminate(); }

        /**
         * @brief Handles the exit out of the coroutine body.
         *
         * This method is called when the coroutine returns. Does nothing.
         *
         * @internal
         * @test @a SimpleTaskTest.Basic
         */
        constexpr void return_void() const noexcept {}

        /**
         * @brief Issues an awaiter for initial suspend point.
         *
         * This method is called to obtain an awaiter for the initial suspend point of the coroutine.
         * By returning `std::suspend_never`, the coroutine starts execution immediately.
         *
         * @return Awaiter for initial suspend point.
         * @retval std::suspend_never The coroutine starts execution immediately.
         *
         * @internal
         * @test @a SimpleTaskTest.Basic
         */
        constexpr auto initial_suspend() noexcept { return std::suspend_never{}; }

        /**
         * @brief Issues an awaiter for final suspend point.
         *
         * This method is called to obtain an awaiter for the final suspend point of the coroutine.
         * By returning `std::suspend_never`, the coroutine does not suspend at the end.
         *
         * @return Awaiter for final suspend point.
         * @retval std::suspend_never The coroutine does not suspend at the end.
         *
         * @internal
         * @test @a SimpleTaskTest.Basic
         */
        constexpr auto final_suspend() noexcept { return std::suspend_never{}; }

        /// @endcond
    };
};
// NOLINTEND(readability-convert-member-functions-to-static)

/**
 * @example eager_task.cpp
 * Usage example for `eager_task`.
 */

/**
 * @brief Turns any awaitable into an eager fire-and-forget coroutine.
 *
 * This method template takes an awaitable callable and its arguments, invokes the callable with the provided arguments,
 * and `co_await`'s the result. It effectively runs the awaitable in a fire-and-forget manner.
 *
 * @tparam Awaitable The type of the awaitable callable.
 * @tparam Args The types of the arguments to be passed to the callable.
 * @param f The awaitable callable to be invoked.
 * @param args The arguments to be passed to the callable.
 * @return A fire-and-forget task.
 * @internal
 * @test @a SimpleTaskTest.RunAwaitable
 */
template<typename Awaitable, typename... Args>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
eager_task run_awaitable(Awaitable&& f, Args&&... args)
{
    co_await std::invoke(std::forward<Awaitable>(f), std::forward<Args>(args)...);
}

/**
 * @example run_awaitable.cpp
 * Shows how to use `run_awaitable` to turn any awaitable into an eager coroutine that runs synchronously from the caller's perspective.
 */

}  // namespace wwa::coro

#endif /* CF9BADEE_8060_4165_91B2_54C840903F88 */
