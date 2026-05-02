#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include <queue>

#include "common.hpp"

// --- The Scheduler ---
// A simple FIFO queue to manage coroutines that are ready to run.
struct Scheduler {
    std::queue<std::coroutine_handle<>> ready_tasks;

    void schedule(std::coroutine_handle<> h) {
        if (h) ready_tasks.push(h);
    }

    void run() {
        while (!ready_tasks.empty()) {
            auto h = ready_tasks.front();
            ready_tasks.pop();
            if (!h.done()) {
                h.resume();
            }
        }
    }
};

// Global scheduler instance for simplicity in this example
Scheduler g_scheduler;

// --- The Task Template ---
template <typename T>
struct task {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    task(handle_type h) : coro(h) {}
    task(task &&other) noexcept : coro(other.coro) { other.coro = nullptr; }
    task(const task &) = delete;
    ~task() { if (coro) coro.destroy(); }

    struct awaiter {
        handle_type coro;

        bool await_ready() const noexcept {
            return coro.done();
        }

        // Return true to suspend and return control to the caller (or scheduler)
        bool await_suspend(std::coroutine_handle<> awaiting_coro) {
            // Store the awaiting coroutine so we can resume it when this task finishes
            coro.promise().continuation = awaiting_coro;
            
            // Schedule the internal coroutine to run
            g_scheduler.schedule(coro);
            return true; 
        }

        T await_resume() {
            if (coro.promise().exception) {
                std::rethrow_exception(coro.promise().exception);
            }
            return coro.promise().value;
        }
    };

    auto operator co_await() & noexcept { return awaiter{coro}; }
    auto operator co_await() const & noexcept { return awaiter{coro}; }
    auto operator co_await() && noexcept { return awaiter{std::move(coro)}; }

    // Logic for top-level manual execution
    void start() {
        if (coro && !coro.done()) {
            g_scheduler.schedule(coro);
        }
    }

    struct promise_type {
        T value;
        std::exception_ptr exception;
        std::coroutine_handle<> continuation = nullptr;

        auto get_return_object() {
            return task{handle_type::from_promise(*this)};
        }

        auto initial_suspend() noexcept { return std::suspend_always{}; }

        // Final suspend returns control to the continuation (the awaiting coroutine)
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(handle_type h) noexcept {
                if (h.promise().continuation) {
                    g_scheduler.schedule(h.promise().continuation);
                }
            }
            void await_resume() noexcept {}
        };

        auto final_suspend() noexcept {
            return final_awaiter{};
        }

        void return_value(T v) noexcept { value = v; }
        void unhandled_exception() noexcept { exception = std::current_exception(); }
    };

    handle_type coro;
};

// --- Coroutines ---
task<int> async_double(int x) {
    co_return compute_double(x);
}

task<int> async_sum(int count) {
    std::vector<task<int>> tasks;
    tasks.reserve(count);

    // 1. Launch/Create all tasks
    // They are created in a suspended state (initial_suspend).
    for (int i = 1; i <= count; ++i) {
        tasks.push_back(async_double(i));
    }

    // 2. Manually trigger them to enter the scheduler
    // This allows them to run "in parallel" within the scheduler's queue.
    for (auto& t : tasks) {
        t.start(); 
    }

    // 3. Await points for all tasks
    int total = 0;
    for (auto& t : tasks) {
        // If the task is already done, co_await returns immediately.
        // If not, async_sum suspends here until this specific task finishes.
        total += co_await t;
    }

    co_return total;
}

int main(int argc, char **argv) {
    int count = (argc > 1) ? std::stoi(argv[1]) : 100;
    
    auto start = std::chrono::steady_clock::now();

    // 1. Create the top-level task
    task<int> job = async_sum(count);
    
    // 2. Kick off the execution by scheduling the root coroutine
    job.start();

    // 3. Enter the scheduler loop
    g_scheduler.run();

    // 4. Retrieve result
    int result = job.coro.promise().value;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();

    std::cout << "result=" << result << "\n";
    std::cout << "elapsed_ms=" << elapsed << "\n";
    return 0;
}