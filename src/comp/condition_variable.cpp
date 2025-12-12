#include "coro/comp/condition_variable.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
    auto cond_var::cv_awaitor::await_resume() noexcept -> void
    {
        if (m_suspend_state) {
            m_ctx.unregister_wait();
        }
    }
    auto cond_var::cv_awaitor::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        m_awaiter_coro = handle;
        return register_lock();
    }

    auto cond_var::cv_awaitor::register_lock() noexcept -> bool
    {
        if (m_cond && m_cond())
        {
            return false;
        }

        if (!m_suspend_state) {
            m_ctx.register_wait();
        }
        m_suspend_state = true;

        register_cv();
        m_mtx.unlock();
        return true;
    }

    auto cond_var::cv_awaitor::register_cv() noexcept -> void
    {
        m_next = nullptr;

        m_cv.m_lock.lock();
        if (m_cv.m_tail == nullptr)
        {
           m_cv.m_head = m_cv.m_tail = this;
        }
        else
        {
            m_cv.m_tail->m_next = this;
            m_cv.m_tail         = this;
        }
        m_cv.m_lock.unlock();
    }

    auto cond_var::cv_awaitor::resume() noexcept -> void
    {
        if (m_cond && !m_cond()) {
            if (!m_suspend_state) {
                m_ctx.register_wait();
            }
            m_suspend_state = true;

            register_cv();
            m_mtx.unlock();
            return;
        }
        mutex_awaiter::resume();
    }

    auto cond_var::cv_awaitor::wake_up() noexcept -> void
    {
        if (!mutex_awaiter::register_waitor()) {
            // got the mutex lock
            resume();
        }
    }

    auto cond_var::notify_one() noexcept -> void
    {
        m_lock.lock();
        auto cur = m_head;
        if (cur != nullptr)
        {
            m_head = reinterpret_cast<cv_awaitor*>(m_head->m_next);
            if (m_head == nullptr)
            {
                m_tail = nullptr;
            }
            m_lock.unlock();
            cur->wake_up();
        }
        else
        {
           m_lock.unlock();
        }
    }

    auto cond_var::notify_all() noexcept -> void
    {
        m_lock.lock();
        auto cur = m_head;
        m_head = nullptr;
        m_tail = nullptr;
        m_lock.unlock();

        while(cur != nullptr) {
            auto next = reinterpret_cast<cv_awaitor*>(cur->m_next);
            cur->wake_up();
            cur = next;
        }
    }
} // namespace coro
