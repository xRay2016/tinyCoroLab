/**
 * @file mutex.hpp
 * @author JiahuiWang
 * @brief lab4d
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>
#include <type_traits>

#include "coro/comp/mutex_guard.hpp"
#include "coro/context.hpp"
#include "coro/detail/types.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab4d, in this part you will build the basic coroutine
 * synchronization component----mutex by modifing mutex.hpp and mutex.cpp.
 * Please ensure you have read the document of lab4d.
 *
 * @warning You should carefully consider whether each implementation should be thread-safe.
 *
 * You should follow the rules below in this part:
 *
 * @note The location marked by todo is where you must add code, but you can also add code anywhere
 * you want, such as function and class definitions, even member variables.
 *
 * @note lab4 and lab5 are free designed lab, leave the interfaces that the test case will use,
 * and then, enjoy yourself!
 */

class context;

using detail::awaiter_ptr;
using std::atomic;
// TODO[lab4d]: This mutex is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the member function and construct function's declaration same with example.

class mutex
{
    // Just make lock_guard() compile success
    struct mutex_awaiter
    {
        mutex_awaiter(context& ctx, mutex& m) noexcept : m_mtx(m), m_ctx(ctx) {}
        auto await_ready() noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;
        auto await_resume() noexcept -> void;

        mutex&                  m_mtx;
        context&                m_ctx;
        mutex_awaiter*          m_next{nullptr};
        std::coroutine_handle<> m_handle{nullptr};
    };

    struct mutex_guard_awaiter : public mutex_awaiter
    {
        using mutex_awaiter::mutex_awaiter;
        auto await_resume() noexcept -> detail::lock_guard<mutex>;
    };

public:
    mutex() noexcept : m_state(m_unlock_state) {}
    ~mutex() noexcept {}

    auto try_lock() noexcept -> bool;

    auto lock() noexcept -> mutex_awaiter { return mutex_awaiter{local_context(), *this}; };

    auto unlock() noexcept -> void;

    auto lock_guard() noexcept -> mutex_guard_awaiter { return mutex_guard_awaiter{local_context(), *this}; };

private:
    auto register_awaiter(awaiter_ptr ptr) noexcept -> bool;

private:
    static inline awaiter_ptr m_unlock_state{nullptr};
    static inline awaiter_ptr m_locked_but_no_waiting{reinterpret_cast<mutex_awaiter*>(1)};
    atomic<awaiter_ptr>       m_state{};
};

}; // namespace coro
