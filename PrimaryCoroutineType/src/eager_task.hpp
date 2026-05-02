#pragma once

#include <coroutine>
#include <exception>

namespace primary {

template <typename T>
struct eager_task {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    eager_task(handle_type h) : coro(h) {}
    eager_task(eager_task &&other) noexcept : coro(other.coro) { other.coro = nullptr; }
    eager_task(const eager_task &) = delete;
    ~eager_task() {
        if (coro) {
            coro.destroy();
        }
    }

    eager_task &operator=(eager_task &&other) noexcept {
        if (this != &other) {
            if (coro) coro.destroy();
            coro = other.coro;
            other.coro = nullptr;
        }
        return *this;
    }

    bool ready() const noexcept {
        return !coro || coro.done();
    }

    T get() {
        while (coro && !coro.done()) {
            coro.resume();
        }
        if (coro.promise().exception) {
            std::rethrow_exception(coro.promise().exception);
        }
        return coro.promise().value;
    }

    struct awaiter {
        handle_type coro;

        bool await_ready() const noexcept {
            return !coro || coro.done();
        }

        bool await_suspend(std::coroutine_handle<>) {
            coro.resume();
            return false;
        }

        T await_resume() {
            if (coro.promise().exception) {
                std::rethrow_exception(coro.promise().exception);
            }
            return coro.promise().value;
        }
    };

    auto operator co_await() & noexcept {
        return awaiter{coro};
    }

    auto operator co_await() && noexcept {
        return awaiter{coro};
    }

    struct promise_type {
        T value;
        std::exception_ptr exception;

        eager_task get_return_object() {
            return eager_task{handle_type::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_value(T v) noexcept {
            value = v;
        }

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    handle_type coro;
};

template <>
struct eager_task<void> {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    eager_task(handle_type h) : coro(h) {}
    eager_task(eager_task &&other) noexcept : coro(other.coro) { other.coro = nullptr; }
    eager_task(const eager_task &) = delete;
    ~eager_task() {
        if (coro) {
            coro.destroy();
        }
    }

    eager_task &operator=(eager_task &&other) noexcept {
        if (this != &other) {
            if (coro) coro.destroy();
            coro = other.coro;
            other.coro = nullptr;
        }
        return *this;
    }

    bool ready() const noexcept {
        return !coro || coro.done();
    }

    void get() {
        while (coro && !coro.done()) {
            coro.resume();
        }
        if (coro.promise().exception) {
            std::rethrow_exception(coro.promise().exception);
        }
    }

    struct awaiter {
        handle_type coro;

        bool await_ready() const noexcept {
            return !coro || coro.done();
        }

        bool await_suspend(std::coroutine_handle<>) {
            coro.resume();
            return false;
        }

        void await_resume() {
            if (coro.promise().exception) {
                std::rethrow_exception(coro.promise().exception);
            }
        }
    };

    auto operator co_await() & noexcept {
        return awaiter{coro};
    }

    auto operator co_await() && noexcept {
        return awaiter{coro};
    }

    struct promise_type {
        std::exception_ptr exception;

        eager_task get_return_object() {
            return eager_task{handle_type::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    handle_type coro;
};

} // namespace primary
