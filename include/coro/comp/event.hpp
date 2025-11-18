/**
 * @file event.hpp
 * @author JiahuiWang
 * @brief lab4a
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <atomic>
#include <coroutine>

#include "coro/attribute.hpp"
#include "coro/concepts/awaitable.hpp"
#include "coro/context.hpp"
#include "coro/detail/container.hpp"
#include "coro/detail/types.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab4a, in this part you will build the basic coroutine
 * synchronization component - event by modifing event.hpp and event.cpp. Please ensure
 * you have read the document of lab4a.
 *
 * @warning You should carefully consider whether each implementation should be thread-safe.
 *
 * You should follow the rules below in this part:
 *
 * @note The location marked by todo is where you must add code, but you can also add code anywhere
 * you want, such as function and class definitions, even member variables.
 *
 * @note lab4 and lab5 are free designed lab, leave the interfaces that the test case will use,
 * and then, enjoy yourself!
 */
class context;

namespace detail
{
// TODO[lab4a]: Add code that you don't want to use externally in namespace detail
}; // namespace detail

// TODO[lab4a]: This event is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the function set() and wait()'s declaration same with example.
class base_awaiter;

class event_base {
public:
    std::atomic<bool> m_is_set{false};
    std::atomic<base_awaiter*> m_head{nullptr};

};

class base_awaiter {

public:
    event_base& m_event;
    context* const m_ctx;
    base_awaiter* m_next{nullptr};
    std::coroutine_handle<> m_handle{nullptr};

    base_awaiter(event_base& ev) noexcept : m_event(ev) , m_ctx(detail::linfo.ctx){}

    auto await_ready() -> bool {
        m_ctx->register_wait();
        if (m_event.m_is_set) {
            return true;
        }
        return false;
    }
    auto await_suspend(std::coroutine_handle<> handle) -> void {
        m_handle = handle;
        base_awaiter* old_head = m_event.m_head;
        do
        {
            m_next = old_head;
        } while(!m_event.m_head.compare_exchange_strong(old_head, this, std::memory_order_acq_rel));
    }
};

template<typename return_type = void>
class event : public event_base, public detail::container<return_type>
{
    // Just make compile success
    class awaiter : public base_awaiter
    {
    public:
        awaiter(event<return_type>& ev) noexcept : base_awaiter(ev) {}
        auto await_resume() -> return_type {
            m_ctx->unregister_wait();
            return static_cast<event<return_type>&>(m_event).result();
        }
    };

public:
    auto wait() noexcept -> awaiter { return awaiter(std::ref(*this)); } // return awaitable

    template<typename value_type>
    auto set(value_type&& value) noexcept -> void
    {
        m_is_set.store(true, std::memory_order_acq_rel);
        this->return_value(std::forward<value_type>(value));
        base_awaiter* curr = m_head.exchange(nullptr, std::memory_order_acquire);
        while (curr != nullptr) {
            base_awaiter* next = curr->m_next;
            curr->m_ctx->submit_task(curr->m_handle);
            curr = next;
        }
    }
};

template<>
class event<> : public event_base {
private:
    // Just make compile success
    class awaiter : public base_awaiter
    {
    public:
        awaiter(event_base& ev) noexcept : base_awaiter(ev) {}

        auto await_resume() -> void {m_ctx->unregister_wait(); }
    };
public:
    auto wait() noexcept -> awaiter{ return awaiter(std::ref(*this)); } // return awaitable
    auto set() noexcept -> void {
        m_is_set.store(true, std::memory_order_acq_rel);
        base_awaiter* curr = m_head.exchange(nullptr, std::memory_order_acquire);
        while(curr != nullptr) {
            base_awaiter* next = curr->m_next;
            curr->m_ctx->submit_task(curr->m_handle);
            curr = next;
        }
    }
};

/**
 * @brief RAII for event
 *
 */
class event_guard
{
    using guard_type = event<>;

public:
    event_guard(guard_type& ev) noexcept : m_ev(ev) {}
    ~event_guard() noexcept { m_ev.set(); }

private:
    guard_type& m_ev;
};

}; // namespace coro
