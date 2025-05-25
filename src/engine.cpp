#include "coro/engine.hpp"
#include "coro/log.hpp"
#include "coro/net/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_acq_rel;
using std::memory_order_acquire;
using std::memory_order_relaxed;
using std::memory_order_release;

auto engine::init() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    linfo.egn = this;

    // init m_upxy
    m_upxy.init(::coro::config::kEntryLength);

    // init m_num_io_wait_submit
    m_num_io_wait_submit = 0;
    m_num_io_running     = 0;
}

auto engine::deinit() noexcept -> void
{
    m_deinited.store(true, std::memory_order_release);
    // TODO[lab2a]: Add you codes
    linfo.egn = nullptr;
    m_upxy.deinit();

    m_num_io_wait_submit = 0;
    m_num_io_running     = 0;

    if (!m_task_queue.was_empty())
    {
        log::warn("task queue isn't empty when deinit");
    }
    mpsc_queue<coroutine_handle<>> task_queue;
    m_task_queue.swap(task_queue);
}

auto engine::ready() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return !m_task_queue.was_empty();
}

auto engine::get_free_urs() noexcept -> ursptr
{
    // TODO[lab2a]: Add you codes
    return m_upxy.get_free_sqe();
}

auto engine::num_task_schedule() noexcept -> size_t
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.was_size();
}

auto engine::schedule() noexcept -> coroutine_handle<>
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.pop();
}

auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2a]: Add you
    assert(handle != nullptr && "engine get nullptr task handle");
    m_task_queue.push(handle);
    wake_up(task_flag);
}

auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule();
    coro.resume();
    if (coro.done())
    {
        clean(coro);
    }
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    if (cqe->user_data == 0)
    {
        log::warn("res={}, flags={}, user_data={}\n", cqe->res, cqe->flags, (long long)cqe->user_data);
    }
    auto data = reinterpret_cast<net::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::poll_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    auto num_task_wait = m_num_io_wait_submit.load(memory_order_acquire);
    if (num_task_wait > 0)
    {
        int num = m_upxy.submit();
        num_task_wait -= num;

        m_num_io_running.fetch_add(num, memory_order_acq_rel);
        m_num_io_wait_submit.fetch_sub(num, memory_order_acq_rel);
    }

    // wait event fd at first
    auto flag = m_upxy.wait_eventfd();
    if (!wake_by_cqe(flag))
    {
        return;
    }

    // peek cqe is non block, so no need to check flag
    auto wait_count = std::min(m_urc.size(), m_num_io_running.load(memory_order_acquire));
    auto num        = m_upxy.peek_batch_cqe(m_urc.data(), wait_count);
    if (num != 0)
    {
        for (int i = 0; i < num; i++)
        {
            handle_cqe_entry(m_urc[i]);
        }
        m_upxy.cq_advance(num);
        m_num_io_running.fetch_sub(num, memory_order_acq_rel);
    }
}

auto engine::add_io_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_num_io_wait_submit.fetch_add(1, memory_order_acq_rel);
    wake_up(io_flag);
}

auto engine::empty_io() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return m_num_io_wait_submit.load(memory_order_acquire) == 0 && m_num_io_running.load(memory_order_acquire) == 0;
}

auto engine::wake_up(uint64_t val) noexcept -> void
{
    m_upxy.write_eventfd(val);
}
}; // namespace coro::detail
