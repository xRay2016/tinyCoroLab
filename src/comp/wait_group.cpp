#include "coro/comp/wait_group.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
// TODO[lab4c] : Add codes if you need
auto wait_group::add(int count) noexcept -> void
{
    m_count.fetch_add(count, memory_order_acq_rel);
}
auto wait_group::done() noexcept -> void
{
    if (m_count.fetch_sub(1, memory_order_acq_rel) <= 1)
    {
        resume_all();
    }
}
auto wait_group::resume_all() noexcept -> void
{
    auto ptr = static_cast<awaiter*>(m_state.exchange(nullptr, memory_order_acq_rel));
    while (ptr != nullptr)
    {
        auto next_awaiter = ptr->m_next;
        ptr->m_ctx.submit_task(ptr->m_handle);
        ptr = next_awaiter;
    }
}
auto wait_group::register_awaiter(awaiter* aw) noexcept -> bool
{
    // set awaiter to list
    detail::awaiter_ptr old_head = nullptr;
    while (true)
    {
        // if already zero wait, return false(not suspend)
        if (m_count.load(memory_order_acquire) <= 0)
        {
            return false;
        }

        old_head   = m_state.load(std::memory_order_acquire);
        aw->m_next = static_cast<awaiter*>(old_head);
        if (m_state.compare_exchange_weak(old_head, static_cast<detail::awaiter_ptr>(aw), memory_order_acq_rel))
        {
            return true;
        }
    }
}

auto wait_group::awaiter::await_ready() noexcept -> bool
{
    m_ctx.register_wait();
    return m_wg.m_count.load(memory_order_acquire) <= 0;
}
auto wait_group::awaiter::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
{
    m_handle = handle;
    return m_wg.register_awaiter(this);
}
auto wait_group::awaiter::await_resume() noexcept -> void
{
    return m_ctx.unregister_wait();
}
}; // namespace coro
