#ifndef BF063C31_B18E_470A_8642_C43A4F1B49FC
#define BF063C31_B18E_470A_8642_C43A4F1B49FC

/**
 * @file detail.h
 * @brief Internal utility functions for coroutine handling.
 *
 * This header file defines a set of internal utility functions used for handling coroutines.
 * These functions are intended to be used within the library to manage coroutine states and ensure proper error handling.
 *
 * @warning The functions declared in this file are not intended for public use.
 */

#include <coroutine>
#include "exceptions.h"

/// @cond INTERNAL

// NOLINTNEXTLINE(modernize-concat-nested-namespaces)
namespace wwa::coro {  // NOSONAR

/**
 * @brief Internal utility functions for coroutine handling.
 *
 * This namespace contains a set of utility functions, types, and base classes that are used internally within the library
 * to manage and handle coroutine tasks. These utilities provide essential checks, operations, and common functionality to ensure
 * that coroutines are used correctly and that errors are handled appropriately.
 *
 * @warning These utilities are designed to be used internally within the library and are not intended for public use.
 */
namespace detail {

/**
 * @brief Checks if a coroutine handle is valid and not done.
 *
 * This function checks if the given coroutine handle is valid and if the coroutine is not done.
 *
 * @param h The coroutine handle to check.
 * @return Coroutine handle status.
 * @retval true The handle is valid and the coroutine is not done.
 * @retval false The handle is invalid or the coroutine is done.
 */
constexpr bool is_good_handle(const std::coroutine_handle<>& h) noexcept
{
    return h && !h.done();
}

/**
 * @brief Checks whether the coroutine has already finished or is not valid.
 *
 * This function checks if the given coroutine handle is invalid (null) or if the coroutine is done.
 * This is the opposite of what `is_good_handle()` does.
 *
 * @param h The coroutine handle to check.
 * @return Readiness of the coroutine.
 * @retval true The handle is invalid (e.g., for an empty coroutine) or the coroutine is done.
 * @retval false The handle is valid and the coroutine is not done.
 */
constexpr bool is_ready(const std::coroutine_handle<>& h) noexcept
{
    return !h || h.done();
}

/**
 * @brief Throws an exception if the coroutine handle is invalid.
 *
 * This function checks if the given coroutine handle is invalid (null) and throws a `bad_task` exception if it is.
 *
 * @param h The coroutine handle to check.
 * @throws bad_task if the coroutine handle is invalid.
 */
inline void check_coroutine(const std::coroutine_handle<>& h)
{
    if (!h) {
        throw bad_task("task is empty or destroyed");
    }
}

}  // namespace detail

}  // namespace wwa::coro

/// @endcond

#endif /* BF063C31_B18E_470A_8642_C43A4F1B49FC */
