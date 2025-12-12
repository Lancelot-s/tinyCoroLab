/**
 * @file condition_variable.hpp
 * @author JiahuiWang
 * @brief lab5b
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <functional>

#include "coro/attribute.hpp"
#include "coro/comp/mutex.hpp"
#include "coro/spinlock.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab5b, in this part you will build the basic coroutine
 * synchronization component����condition_variable by modifing condition_variable.hpp
 * and condition_variable.cpp. Please ensure you have read the document of lab5b.
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

using cond_type = std::function<bool()>;

class condition_variable;
using cond_var = condition_variable;

// TODO[lab5b]: This condition_variable is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the member function and construct function's declaration same with example.
class condition_variable final
{
public:
    condition_variable() noexcept  = default;
    ~condition_variable() noexcept = default;

    CORO_NO_COPY_MOVE(condition_variable);

    struct cv_awaitor : public coro::mutex::mutex_awaiter
    {
        cond_var& m_cv;
        cond_type m_cond;
        bool m_suspend_state;
        cv_awaitor(cond_var& cv, mutex& mtx, cond_type& cond) noexcept
            : mutex::mutex_awaiter(mtx), m_cv(cv), m_cond(cond), m_suspend_state(false) {}

        cv_awaitor(cond_var& cv, mutex& mtx) noexcept
            : mutex::mutex_awaiter(mtx), m_cv(cv), m_suspend_state(false) {}

        auto await_ready() noexcept -> bool { return false;}

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;

        auto await_resume() noexcept -> void;

        auto resume() noexcept -> void override;

        auto register_cv() noexcept -> void;

        auto register_lock() noexcept -> bool;

        auto wake_up() noexcept -> void;

    };

    auto wait(mutex& mtx) noexcept -> cv_awaitor { return cv_awaitor(std::ref(*this), mtx);}

    auto wait(mutex& mtx, cond_type&& cond) noexcept -> cv_awaitor { return cv_awaitor(std::ref(*this), mtx, cond); }

    auto wait(mutex& mtx, cond_type& cond) noexcept -> cv_awaitor { return cv_awaitor(std::ref(*this), mtx, cond); }

    auto notify_one() noexcept -> void;

    auto notify_all() noexcept -> void;

private:
    //friend cv_awaitor;
    detail::spinlock m_lock;
    alignas(config::kCacheLineSize) cv_awaitor* m_head{nullptr};
    alignas(config::kCacheLineSize) cv_awaitor* m_tail{nullptr};
};

}; // namespace coro
