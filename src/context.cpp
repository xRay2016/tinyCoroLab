#include "coro/context.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
context::context() noexcept
{
    m_id = ginfo.context_id.fetch_add(1, std::memory_order_relaxed);
}

auto context::init() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = this;

    m_engine.init();
    m_reference_count.store(0, std::memory_order_release);
}

auto context::deinit() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = nullptr;

    m_engine.deinit();
    m_reference_count.store(0, std::memory_order_release);
}

auto context::start() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job = make_unique<jthread>(
        [this](stop_token token)
        {
            this->init();
            if (m_stop_cb == nullptr)
            {
                m_stop_cb = [this]()
                {
                    log::info("call default stop cb");
                    this->notify_stop();
                };
            }
            this->run(token);
            this->deinit();
        });
}

auto context::notify_stop() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job->request_stop();
    // if there is no task running, the engine will be suspended
    // so wake up here
    m_engine.wake_up(engine::task_flag);
}

auto context::submit_task(std::coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.submit_task(handle);
}

auto context::register_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_reference_count.fetch_add(register_cnt, memory_order_acq_rel);
}

auto context::unregister_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_reference_count.fetch_sub(register_cnt, memory_order_acq_rel);
}

auto context::run(stop_token token) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    while (!token.stop_requested())
    {
        auto num = m_engine.num_task_schedule();
        for (auto i = 0; i < num; i++)
        {
            m_engine.exec_one_task();
        }

        if (m_engine.empty_io() && m_reference_count.load(memory_order_acquire) == 0)
        {
            if (!m_engine.ready())
            {
                m_stop_cb();
            }
            else
            {
                continue;
            }
        }

        m_engine.poll_submit();
    }
}

}; // namespace coro