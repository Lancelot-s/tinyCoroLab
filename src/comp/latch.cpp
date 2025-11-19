#include "coro/comp/latch.hpp"

namespace coro
{
// TODO[lab4b]: Add codes if you need
    auto latch::count_down() noexcept -> void
    {
        if (m_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            m_event.set();
        }
    }

    auto latch::wait() noexcept -> event<>::awaiter
    {
        return event<>::awaiter{this->m_event};
    }
};
