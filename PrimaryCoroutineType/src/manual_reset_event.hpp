#pragma once

#include <coroutine>

namespace primary {

struct manual_reset_event {
    bool signaled = false;
    std::coroutine_handle<> waiter = nullptr;

    bool await_ready() const noexcept {
        return signaled;
    }

    bool await_suspend(std::coroutine_handle<> h) noexcept {
        waiter = h;
        return true;
    }

    void await_resume() const noexcept {}

    void signal() noexcept {
        signaled = true;
        if (waiter) {
            auto w = waiter;
            waiter = nullptr;
            w.resume();
        }
    }
};

} // namespace primary
