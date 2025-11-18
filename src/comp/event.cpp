#include "coro/comp/event.hpp"
#include "coro/scheduler.hpp"

namespace coro::detail
{
// TODO[lab4a] : Add codes if you need
auto event_base::_set_state() noexcept -> void
{
    auto old_value = m_waiter_head.exchange(this, std::memory_order_acq_rel);
    if (old_value != this) {
        auto waiter_head = static_cast<base_awaiter*>(old_value);
        _resume_all_awaiter(waiter_head);
    }
}

auto event_base::_resume_all_awaiter(base_awaiter* awaiter) noexcept ->void
{
    while (awaiter != nullptr) {
        auto next = static_cast<base_awaiter*>(awaiter->m_next);
        awaiter->m_ctx.submit_task(awaiter->m_await_coro);
        awaiter = next;
    }
}

auto event_base::registe_awaiter(base_awaiter* awaiter) noexcept -> bool
{
    awaiter_ptr old_value = m_waiter_head.load(std::memory_order_acquire);
    do {
        if (old_value == this){
            return false;
        }
        awaiter->m_next = static_cast<base_awaiter*>(old_value);
    } while(!m_waiter_head.compare_exchange_weak(old_value, awaiter, std::memory_order_acq_rel));
    return true;
}

auto event_base::base_awaiter::await_ready() noexcept -> bool
{
    m_ctx.register_wait();
    return m_event.is_set();
}

auto event_base::base_awaiter::await_suspend(std::coroutine_handle<> handle) noexcept -> bool
{
    m_await_coro = handle;
    return m_event.registe_awaiter(this);
}

}