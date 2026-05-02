#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "coroutine_types.hpp"

using namespace primary;

// Eager task: body begins immediately when the coroutine function is called.
eager_task<int> eager_double(int x) {
    std::cout << "[eager] created for " << x << "\n";
    co_return x * 2;
}

eager_task<int> eager_sum(int count) {
    std::cout << "[eager_sum] started immediately\n";
    int total = 0;
    for (int i = 1; i <= count; ++i) {
        total += co_await eager_double(i);
    }
    co_return total;
}

// Lazy task: nothing happens until resume() or co_await.
lazy_task<int> lazy_double(int x) {
    std::cout << "[lazy] body running for " << x << "\n";
    co_return x * 2;
}

lazy_task<int> lazy_sum(int count) {
    std::cout << "[lazy_sum] body begins only after resume() or co_await\n";
    int total = 0;
    for (int i = 1; i <= count; ++i) {
        total += co_await lazy_double(i);
    }
    co_return total;
}

// Fire-and-forget: the caller cannot await or get a result back.
fire_and_forget background_log(std::string message) {
    std::cout << "[fire_and_forget] start: " << message << "\n";
    co_await sleep_for(std::chrono::milliseconds(100));
    std::cout << "[fire_and_forget] done: " << message << "\n";
}

// Generator: produces a sequence of values lazily.
generator<int> count_range(int start, int end) {
    for (int i = start; i < end; ++i) {
        co_yield i;
    }
}

// Signal-awaitable: resumes when an external event signals it.
lazy_task<void> wait_for_signal(manual_reset_event &event) {
    std::cout << "[signal] waiting for event...\n";
    co_await event;
    std::cout << "[signal] event received\n";
}

int main() {
    std::cout << "=== Eager Task ===\n";
    auto eager_job = eager_sum(3);
    std::cout << "eager ready: " << eager_job.ready() << "\n";
    std::cout << "eager result: " << eager_job.get() << "\n\n";

    std::cout << "=== Lazy Task ===\n";
    auto lazy_job = lazy_sum(3);
    std::cout << "lazy ready before resume: " << lazy_job.ready() << "\n";
    lazy_job.resume();
    std::cout << "lazy ready after resume: " << lazy_job.ready() << "\n";
    std::cout << "lazy result: " << lazy_job.get() << "\n\n";

    std::cout << "=== Fire-and-Forget ===\n";
    background_log("write-debug");
    std::cout << "main continues without waiting\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    std::cout << "main awakened after background work\n\n";

    std::cout << "=== Generator ===\n";
    for (int value : count_range(5, 8)) {
        std::cout << "generator yielded: " << value << "\n";
    }
    std::cout << "\n";

    std::cout << "=== Signal Awaitable ===\n";
    manual_reset_event event;
    auto waiter = wait_for_signal(event);
    std::cout << "signal event before starting waiter\n";
    event.signal();
    waiter.get();
    std::cout << "signal example complete\n";

    return 0;
}
