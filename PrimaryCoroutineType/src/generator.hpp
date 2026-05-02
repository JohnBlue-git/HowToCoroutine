#pragma once

#include <coroutine>

namespace primary {

template <typename T>
struct generator {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct iterator;
    struct sentinel {};

    generator(handle_type h) : coro(h) {}
    generator(generator &&other) noexcept : coro(other.coro) { other.coro = nullptr; }
    generator(const generator &) = delete;
    ~generator() {
        if (coro) {
            coro.destroy();
        }
    }

    iterator begin() {
        if (coro && !coro.done()) {
            coro.resume();
        }
        return iterator{coro};
    }

    sentinel end() noexcept {
        return {};
    }

    struct iterator {
        handle_type coro;

        iterator(handle_type h) : coro(h) {}

        iterator &operator++() {
            coro.resume();
            return *this;
        }

        T operator*() const noexcept {
            return coro.promise().current;
        }

        bool operator==(sentinel) const noexcept {
            return !coro || coro.done();
        }

        bool operator!=(sentinel s) const noexcept {
            return !(*this == s);
        }
    };

    struct promise_type {
        T current;

        auto get_return_object() {
            return generator{handle_type::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        std::suspend_always yield_value(T value) noexcept {
            current = value;
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            std::terminate();
        }
    };

    handle_type coro;
};

} // namespace primary
