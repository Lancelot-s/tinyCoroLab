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

namespace detail
{
// TODO[lab4a]: Add code that you don't want to use externally in namespace detail
class event_base {
using awaiter_ptr = void*;
public:
    struct base_awaiter
    {
        base_awaiter(event_base& event) noexcept : m_event(event), m_ctx(local_context()){}

        auto await_ready() noexcept -> bool;

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;

        inline auto await_resume() noexcept -> void {
            m_ctx.unregister_wait();
        }

        event_base&                m_event;
        base_awaiter*              m_next{nullptr};
        coro::context&       m_ctx;
        std::coroutine_handle<>    m_await_coro{nullptr};
    };

    event_base(bool initial_set = false) noexcept : m_waiter_head(initial_set ? this : nullptr) {}
    ~event_base() noexcept = default;

    event_base(const event_base&)               = delete;
    event_base(event_base&&)                    = delete;
    event_base& operator=(const event_base&)    = delete;
    event_base& operator=(event_base&&)         = delete;

public:
    inline auto is_set() const noexcept -> bool { return m_waiter_head.load(std::memory_order_acquire) == this; }

    auto registe_awaiter(base_awaiter* awaiter) noexcept -> bool;

protected:
    auto _resume_all_awaiter(base_awaiter* awaiter) noexcept -> void;

    auto _set_state() noexcept -> void;

private:
    std::atomic<awaiter_ptr> m_waiter_head{nullptr};
};

}; // namespace detail

// TODO[lab4a]: This event is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the function set() and wait()'s declaration same with example.

using coro::detail::event_base;
template<typename return_type = void>
class event : public event_base, public detail::container<return_type>
{
private:
    struct awaiter : public event_base::base_awaiter
    {
        using base_awaiter::base_awaiter;
        auto await_resume() -> decltype(auto) {
            detail::event_base::base_awaiter::await_resume();
            return static_cast<event&>(m_event).result();
        }
    };

public:
    [[CORO_AWAIT_HINT]] inline auto wait() noexcept -> awaiter { return awaiter(std::ref(*this)); } // return awaitable

    template<typename value_type>
    auto set(value_type&& value) noexcept -> void {
        this->return_value(std::forward<value_type>(value));
        _set_state();
    }

};

template<>
class event<> : public event_base {
public:
    struct awaiter : public base_awaiter
    {
        using base_awaiter::base_awaiter;
    };
public:
    [[CORO_AWAIT_HINT]] auto wait() noexcept -> awaiter{ return awaiter(std::ref(*this)); } // return awaitable
    auto set() noexcept -> void {
        _set_state();
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
