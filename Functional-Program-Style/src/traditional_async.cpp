#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "common.hpp"

std::thread async_double(int x, std::function<void(int)> callback) {
    return std::thread([x, callback = std::move(callback)]() mutable {
        int result = compute_double(x);
        callback(result);
    });
}

int async_sum(int count) {
    std::vector<std::thread> workers;
    workers.reserve(count);
    std::mutex mutex;
    int total = 0;

    for (int i = 1; i <= count; ++i) {
        workers.emplace_back(async_double(i, [&](int v) {
            std::lock_guard lock(mutex);
            total += v;
        }));
    }

    // This is effectively the same as compute_sum(count) but with callback-driven async semantics.

    for (auto &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    return total;
}

int main(int argc, char **argv) {
    int count = (argc > 1) ? std::stoi(argv[1]) : 100;
    if (count <= 0) {
        std::cout << "result=0\n";
        return 0;
    }

    auto start = std::chrono::steady_clock::now();
    int result = async_sum(count);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();

    std::cout << "result=" << result << "\n";
    std::cout << "elapsed_ms=" << elapsed << "\n";
    return 0;
}
