#include "coro/comp/mutex.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
// TODO[lab4d] : Add codes if you need
    auto mutex::try_lock() noexcept -> bool
    {
        awaiter_ptr unlocked_state = this;
        return m_awaiter_head.compare_exchange_weak(unlocked_state, nullptr, std::memory_order_acq_rel);
    }

    auto mutex::lock() noexcept -> mutex::mutex_awaiter
    {
        return mutex::mutex_awaiter{std::ref(*this)};
    }

    auto mutex::unlock() noexcept -> void
    {
        auto head_ptr = m_awaiter_head.load(std::memory_order_acquire);
        while (true)
        {
            if (head_ptr == this) { //mutex is already unlocked
                break;
            }
            if (head_ptr == nullptr) { //no waitor yet
                if (m_awaiter_head.compare_exchange_weak(head_ptr, this, std::memory_order_acq_rel)) {
                    break;
                } else {
                    continue; // CAS failed, retry
                }
            } 
             // there is at least one waitor
            auto waitor = static_cast<mutex_awaiter*>(head_ptr);
            auto next_waitor = waitor->m_next;
            if (m_awaiter_head.compare_exchange_weak(head_ptr, next_waitor, std::memory_order_acq_rel)) {
                // resume waitor
                waitor->m_ctx.submit_task(waitor->m_awaiter_coro);
                break;
            }
        }
    }

//   below is awaiter
    auto mutex::mutex_awaiter::await_ready() noexcept -> bool
    {
        m_ctx.register_wait();
        return m_mtx.try_lock();
    }

    auto mutex::mutex_awaiter::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        m_awaiter_coro = handle;
        auto old_value = m_mtx.m_awaiter_head.load(std::memory_order_acquire);
        do
        {
            m_next = static_cast<mutex::mutex_awaiter*>(old_value);

            if (m_mtx.try_lock()) {
                //m_mtx.m_awaiter_head = this;
                //m_next = nullptr;
                return false;
            }
        } while (!m_mtx.m_awaiter_head.compare_exchange_weak(old_value, this, std::memory_order_acq_rel));
        return true;
    }

    auto mutex::mutex_awaiter::await_resume() noexcept -> void
    {
        m_ctx.unregister_wait();
    }

}; // namespace coro