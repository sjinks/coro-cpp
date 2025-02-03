#ifndef A271EFBC_A65F_4947_8E87_D4194949A073
#define A271EFBC_A65F_4947_8E87_D4194949A073

/**
 * @file exceptions.h
 * @brief Custom exception classes for coroutine handling.
 *
 * This header file defines custom exception classes used for handling errors related to coroutines.
 * These exceptions are intended to provide more specific error information when dealing with coroutine-related issues.
 */

#include <stdexcept>

namespace wwa::coro {

/**
 * @brief Exception thrown when accessing a result that is not available.
 *
 * The `bad_result_access` exception is thrown when an attempt is made to access a result that is not available.
 * This typically occurs when trying to access the result of a coroutine that has not yet completed or has failed
 * or when trying to iterate or dereference the value past the end of a generator.
 */
class bad_result_access : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

/**
 * @brief Exception thrown when a coroutine task is invalid.
 * 
 * The `bad_task` exception is thrown when an operation is attempted on an invalid coroutine task.
 * This can occur if the task is empty or has been destroyed.
 */
class bad_task : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

}  // namespace wwa::coro

#endif /* A271EFBC_A65F_4947_8E87_D4194949A073 */
