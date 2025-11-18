#include "coro/engine.hpp"
#include "coro/io/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

auto engine::init() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    linfo.egn = this;
    m_upxy.init(config::kEntryLength);

    m_io_wait_submit = 0;
    m_io_complete_submit = 0;
    m_io_running_submit = 0;
}

auto engine::deinit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_upxy.deinit();
    m_io_wait_submit = 0;
    m_io_complete_submit = 0;
    m_io_running_submit = 0;
    linfo.egn = nullptr;
    if (!m_task_queue.was_empty())
    {
        log::warn("engine {} deinit with unhandled tasks", m_id);
    }
    mpmc_queue<coroutine_handle<>> tmp_queue;
    m_task_queue.swap(tmp_queue);
    m_urc.fill(nullptr);
}

auto engine::ready() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return !m_task_queue.was_empty();
}

auto engine::get_free_urs() noexcept -> ursptr
{
    // TODO[lab2a]: Add you codes
    auto sqe = m_upxy.get_free_sqe();

    return sqe;
}

auto engine::num_task_schedule() noexcept -> size_t
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.was_size();
}

auto engine::schedule() noexcept -> coroutine_handle<>
{
    // TODO[lab2a]: Add you codes
    if (!m_task_queue.was_empty()) {
        return m_task_queue.pop();
    }
    return std::noop_coroutine();
}

auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2a]: Add you codes
    assert(handle != nullptr && "engine get nullptr task handle");
    if (m_task_queue.was_full()) {
        return;
    }
    m_task_queue.push(handle);
    wake_up();
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
    auto data = reinterpret_cast<io::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::poll_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_upxy.submit();
    m_io_running_submit += m_io_wait_submit;
    m_io_wait_submit = 0;

    m_upxy.wait_eventfd();
    auto complete_count = m_upxy.handle_for_each_cqe([this](urcptr cqe) {
        handle_cqe_entry(cqe);
    }, true);
    m_io_running_submit -= complete_count;
    m_io_complete_submit += complete_count;
}

auto engine::add_io_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_io_wait_submit++;
}

auto engine::empty_io() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    if (m_io_running_submit + m_io_wait_submit > 0) {
        return false;
    }
    return true;
}
}; // namespace coro::detail
