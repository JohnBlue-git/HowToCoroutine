#pragma once

#include <coroutine>

namespace primary {

struct fire_and_forget {
    struct promise_type {
        fire_and_forget get_return_object() noexcept {
            return {};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_never final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            std::terminate();
        }
    };
};

} // namespace primary
