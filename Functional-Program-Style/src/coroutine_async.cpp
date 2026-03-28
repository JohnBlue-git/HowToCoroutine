#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

template <typename T>
struct task {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    task(handle_type h) : coro(h) {}
    task(task &&other) noexcept : coro(other.coro) { other.coro = nullptr; }
    task(const task &) = delete;
    ~task() {
        if (coro) {
            coro.destroy();
        }
    }

    task &operator=(task &&other) noexcept {
        if (this != &other) {
            if (coro) coro.destroy();
            coro = other.coro;
            other.coro = nullptr;
        }
        return *this;
    }

    bool ready() const { return !coro || coro.done(); }

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
            return coro.done();
        }

        bool await_suspend(std::coroutine_handle<> awaiting) {
            coro.resume();
            // resume awaiting coroutine after downstream coroutine completes.
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

        auto get_return_object() {
            return task{handle_type::from_promise(*this)};
        }

        auto initial_suspend() noexcept {
            return std::suspend_always{};
        }

        auto final_suspend() noexcept {
            return std::suspend_always{};
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

#include "common.hpp"

task<int> async_double(int x) {
    co_return compute_double(x);
}

task<int> async_sum(int count) {
    int total = 0;
    for (int i = 1; i <= count; ++i) {
        int doubled = co_await async_double(i);
        total += doubled;
    }
    co_return total;
}


int main(int argc, char **argv) {
    int count = (argc > 1) ? std::stoi(argv[1]) : 100;
    if (count <= 0) {
        std::cout << "result=0\n";
        return 0;
    }

    auto start = std::chrono::steady_clock::now();
    task<int> job = async_sum(count);
    int result = job.get();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();

    std::cout << "result=" << result << "\n";
    std::cout << "elapsed_ms=" << elapsed << "\n";
    return 0;
}
