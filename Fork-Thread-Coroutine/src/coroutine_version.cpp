#include <algorithm>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <deque>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "common.hpp"

namespace {

class Scheduler;

class Task {
public:
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };

    explicit Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
    Task(Task&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task& operator=(Task&&) = delete;

    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    std::coroutine_handle<> release() {
        auto h = handle_;
        handle_ = nullptr;
        return h;
    }

private:
    std::coroutine_handle<promise_type> handle_;
};

class Scheduler {
public:
    void schedule(std::coroutine_handle<> handle) {
        queue_.push_back(handle);
    }

    void run() {
        while (!queue_.empty()) {
            auto handle = queue_.front();
            queue_.pop_front();
            handle.resume();
            if (handle.done()) {
                handle.destroy();
            } else {
                queue_.push_back(handle);
            }
        }
    }

private:
    std::deque<std::coroutine_handle<>> queue_;
};

Task worker(
    std::vector<uint64_t>& results,
    std::size_t start,
    std::size_t end,
    uint32_t times,
    std::size_t yield_every
) {
    for (std::size_t idx = start; idx < end; ++idx) {
        results[idx] = apply_fun_multiple_times(static_cast<uint64_t>(idx + 1), times);
        if (yield_every > 0 && ((idx - start + 1) % yield_every == 0)) {
            co_await std::suspend_always{};
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::size_t count = (argc > 1) ? static_cast<std::size_t>(std::stoull(argv[1])) : 1000000ULL;
    const uint32_t times = (argc > 2) ? static_cast<uint32_t>(std::stoul(argv[2])) : 20U;

    if (count == 0) {
        std::cout << "checksum=0\n";
        return 0;
    }

    unsigned int worker_count = std::thread::hardware_concurrency();
    if (worker_count == 0) {
        worker_count = 4;
    }
    worker_count = std::min<unsigned int>(worker_count, static_cast<unsigned int>(count));

    const std::size_t chunk = (count + worker_count - 1) / worker_count;
    std::vector<uint64_t> results(count, 0);
    Scheduler scheduler;
    std::vector<Task> tasks;
    tasks.reserve(worker_count);

    auto started = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < worker_count; ++i) {
        const std::size_t start = i * chunk;
        const std::size_t end = std::min(count, start + chunk);
        if (start >= end) {
            continue;
        }

        tasks.push_back(worker(results, start, end, times, 1024));
        scheduler.schedule(tasks.back().release());
    }

    scheduler.run();

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started
    ).count();

    std::cout << "checksum=" << checksum(results) << "\n";
    std::cout << "elapsed_ms=" << elapsed_ms << "\n";
    return 0;
}
