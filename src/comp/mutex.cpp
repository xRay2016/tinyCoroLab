#include "coro/comp/mutex.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
// TODO[lab4d] : Add codes if you need

auto mutex::try_lock() noexcept -> bool
{
    // lock failed, return false directly
    return m_state.compare_exchange_strong(m_unlock_state, m_locked_but_no_waiting, memory_order_acq_rel);
}
auto mutex::unlock() noexcept -> void
{
    while (true)
    {
        auto state = m_state.load(memory_order_acquire);
        if (state == m_locked_but_no_waiting)
        {
            // change state to unlock
            if (m_state.compare_exchange_weak(state, m_unlock_state, memory_order_acq_rel))
            {
                return;
            }
        }
        else
        {
            auto mutex_awaiter_ptr      = static_cast<mutex_awaiter*>(state);
            auto mutex_awaiter_next_ptr = static_cast<detail::awaiter_ptr>(mutex_awaiter_ptr->m_next);
            // remove first awaiter
            if (m_state.compare_exchange_weak(state, mutex_awaiter_next_ptr, memory_order_acq_rel))
            {
                // resume current awaiter
                mutex_awaiter_ptr->m_next = nullptr;
                mutex_awaiter_ptr->m_ctx.submit_task(mutex_awaiter_ptr->m_handle);
                return;
            }
        }
    }
}

auto mutex::register_awaiter(coro::detail::awaiter_ptr ptr) noexcept -> bool
{
    auto mutex_awaiter_ptr = static_cast<mutex_awaiter*>(ptr);
    while (true)
    {
        auto state = m_state.load(memory_order_acquire);
        // m_unlock_state just lock
        if (state == m_unlock_state)
        {
            mutex_awaiter_ptr->m_next = nullptr;
            if (m_state.compare_exchange_weak(state, m_locked_but_no_waiting, memory_order_acq_rel))
            {
                return false;
            }
        }
        else
        {
            mutex_awaiter_ptr->m_next = static_cast<mutex_awaiter*>(state);
            if (m_state.compare_exchange_weak(state, mutex_awaiter_ptr, memory_order_acq_rel))
            {
                return true;
            }
        }
    }
}

auto mutex::mutex_awaiter::await_ready() noexcept -> bool
{
    return false;
}
auto mutex::mutex_awaiter::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
{
    m_handle = handle;
    m_ctx.register_wait();
    return m_mtx.register_awaiter(static_cast<awaiter_ptr>(this));
}
auto mutex::mutex_awaiter::await_resume() noexcept -> void
{
    m_ctx.unregister_wait();
}
auto mutex::mutex_guard_awaiter::await_resume() noexcept -> detail::lock_guard<mutex>
{
    mutex_awaiter::await_resume();
    return detail::lock_guard<mutex>(m_mtx);
}
}; // namespace coro