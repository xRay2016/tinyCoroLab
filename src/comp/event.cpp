#include "coro/comp/event.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
// TODO[lab4a] : Add codes if you need
namespace detail
{
auto event_base::awaiter_base::await_ready() noexcept -> bool
{
    m_ctx.register_wait();
    // if set, return ready=true
    return m_event.is_set();
}

auto event_base::awaiter_base::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
{
    m_await_coro = handle;
    return m_event.register_awaiter(this);
}

auto event_base::awaiter_base::await_resume() noexcept -> void
{
    m_ctx.unregister_wait();
}

auto event_base::resume_all(awaiter_base* waiter) noexcept -> void
{
    while (waiter != nullptr)
    {
        auto cur_next = waiter->m_next;
        waiter->m_ctx.submit_task(waiter->m_await_coro);
        waiter = cur_next;
    }
}

auto event_base::set_state() noexcept -> void
{
    auto flag = m_state.exchange(this, std::memory_order_acq_rel);
    if (flag != this)
    {
        auto waiter = static_cast<awaiter_base*>(flag);
        resume_all(waiter);
    }
}

auto event_base::register_awaiter(coro::detail::event_base::awaiter_base* waiter) noexcept -> bool
{
    awaiter_ptr old_ptr = nullptr;
    do
    {
        old_ptr = m_state.load(std::memory_order_acquire);

        if (old_ptr == this)
        {
            // return false to make coroutine run again
            waiter->m_next = nullptr;
            return false;
        }
        waiter->m_next = static_cast<awaiter_base*>(old_ptr);
    } while (!m_state.compare_exchange_weak(old_ptr, waiter, std::memory_order_acq_rel));
    return true;
}

}; // namespace detail

}; // namespace coro