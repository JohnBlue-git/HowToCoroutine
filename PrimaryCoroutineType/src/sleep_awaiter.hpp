#pragma once

#include <coroutine>
#include <thread>
#include <chrono>

namespace primary {

struct sleep_awaiter {
    std::chrono::milliseconds duration;

    bool await_ready() const noexcept {
        return duration.count() <= 0;
    }

    void await_suspend(std::coroutine_handle<> h) const {
        std::thread([h, duration = duration]() mutable {
            std::this_thread::sleep_for(duration);
            h.resume();
        }).detach();
    }

    void await_resume() const noexcept {}
};

inline sleep_awaiter sleep_for(std::chrono::milliseconds duration) noexcept {
    return sleep_awaiter{duration};
}

} // namespace primary
