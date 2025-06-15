#include "coro/scheduler.hpp"

namespace coro
{
auto scheduler::init_impl(size_t ctx_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    detail::init_meta_info();
    m_ctx_cnt = ctx_cnt;
    m_ctxs    = detail::ctx_container{};
    m_ctxs.reserve(m_ctx_cnt);
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs.emplace_back(std::make_unique<context>());
    }
    m_ctx_stop_flag = stop_flag_type(m_ctx_cnt, atomic_ref_wrapper<int>{.val = 1});
    m_stop_token    = static_cast<int>(m_ctx_cnt);
    m_dispatcher.init(m_ctx_cnt, &m_ctxs);
}

auto scheduler::loop_impl() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    // start all context at first
    for (auto i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->set_stop_cb(
            [&, i]()
            {
                auto flag = std::atomic_ref(this->m_ctx_stop_flag[i].val).fetch_and(0, memory_order_acq_rel);
                // only stop once by each context
                // fetch_sub return origin value of m_stop_token
                // if flag = 0, stop_impl is called when m_stop_token is already 0
                // if flag = 1, stop_impl is called when m_stop_token is 1
                if (flag != 0 && this->m_stop_token.fetch_sub(flag, memory_order_acq_rel) == flag)
                {
                    this->stop_impl();
                }
            });
        m_ctxs[i]->start();
    }

    // wait all context
    for (int i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->join();
    }
}

auto scheduler::stop_impl() noexcept -> void
{
    // TODO[lab2b]: example function
    // This is an example which just notify stop signal to each context,
    // if you don't need this, function just ignore or delete it
    // wait all context finished
    //    log::info("call stop_impl");
    for (auto i = 0; i < m_ctx_cnt; i++)
    {
        m_ctxs[i]->notify_stop();
    }
}

auto scheduler::submit_task_impl(std::coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    assert(this->m_stop_token.load(memory_order_acquire) != 0 && "error! submit task after scheduler loop finish");
    size_t ctx_id = m_dispatcher.dispatch();
    m_stop_token.fetch_add(
        1 - std::atomic_ref(m_ctx_stop_flag[ctx_id].val).fetch_or(1, memory_order_acq_rel), memory_order_acq_rel);
    m_ctxs[ctx_id]->submit_task(handle);
}
}; // namespace coro
