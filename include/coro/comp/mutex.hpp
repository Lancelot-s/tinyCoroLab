/**
 * @file mutex.hpp
 * @author JiahuiWang
 * @brief lab4d
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>
#include <type_traits>

#include "coro/comp/mutex_guard.hpp"
#include "coro/context.hpp"
#include "coro/detail/types.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab4d, in this part you will build the basic coroutine
 * synchronization component----mutex by modifing mutex.hpp and mutex.cpp.
 * Please ensure you have read the document of lab4d.
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
// TODO[lab4d]: This mutex is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the member function and construct function's declaration same with example.
class mutex
{
    using awaiter_ptr = void*;
    // Just make lock_guard() compile success
    struct mutex_awaiter
    {
        mutex_awaiter(mutex& mtx) noexcept : m_mtx(mtx), m_ctx(local_context()) {};
        auto await_ready() noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;
        auto await_resume() noexcept -> void;
        mutex_awaiter* m_next{nullptr};
        mutex& m_mtx;
        context& m_ctx;
        std::coroutine_handle<> m_awaiter_coro{nullptr};
    };

    struct guard_awaiter : mutex_awaiter
    {
        using mutex_awaiter::mutex_awaiter;
        auto   await_resume() -> detail::lock_guard<mutex> { m_ctx.unregister_wait(); return detail::lock_guard<mutex>(m_mtx); }
    };

public:
    std::mutex m_lock_mutex;
    mutex() noexcept {}
    ~mutex() noexcept {}

    auto try_lock() noexcept -> bool;

    auto lock() noexcept -> mutex::mutex_awaiter;

    auto unlock() noexcept -> void;

    auto lock_guard() noexcept -> guard_awaiter { return {*this}; };

    auto is_unlock() noexcept -> bool
    {
        return m_awaiter_head.load(std::memory_order_acquire) == this;
    }

    std::atomic<awaiter_ptr> m_awaiter_head{this};
};

}; // namespace coro
