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
    linfo.ctx = this;
    m_engine.init();
    // TODO[lab2b]: Add you codes
}

auto context::deinit() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = nullptr;
    m_engine.deinit();
}

auto context::start() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job = make_unique<jthread>(
        [this](stop_token token)
        {
            this->init();
            if (m_stop_cb == nullptr) {
                m_stop_cb = [&]() { this->notify_stop(); };
            }
            this->run(token);
            this->deinit();
        });
}

auto context::notify_stop() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    if (m_job) {
        m_job->request_stop();
        m_engine.wake_up();
    }
}

auto context::submit_task(std::coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.submit_task(handle);
}

auto context::register_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_register_num.fetch_add(register_cnt, std::memory_order_relaxed);
}

auto context::unregister_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_register_num.fetch_sub(register_cnt, std::memory_order_relaxed);
}

auto context::run(stop_token token) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    while (!token.stop_requested()) {

        auto taskNum = m_engine.num_task_schedule();
        for (size_t i = 0; i < taskNum; i++) {
            m_engine.exec_one_task();
        }

        if (m_engine.empty_io() && m_register_num.load(std::memory_order_relaxed) == 0) {
            if (!m_engine.ready()) {
                m_stop_cb();
            } else {
                continue;
            }
        }

        m_engine.poll_submit();

    }
}

}; // namespace coro