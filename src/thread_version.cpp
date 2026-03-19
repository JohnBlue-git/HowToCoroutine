#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

uint64_t apply_fun_multiple_times(uint64_t x, uint32_t times) {
    for (uint32_t i = 0; i < times; ++i) {
        x *= 2;
    }
    return x;
}

uint64_t checksum(const std::vector<uint64_t>& values) {
    uint64_t sum = 0;
    for (uint64_t value : values) {
        sum ^= value + 0x9e3779b97f4a7c15ULL + (sum << 6U) + (sum >> 2U);
    }
    return sum;
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
    std::vector<std::thread> workers;
    workers.reserve(worker_count);

    auto started = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < worker_count; ++i) {
        const std::size_t start = i * chunk;
        const std::size_t end = std::min(count, start + chunk);
        if (start >= end) {
            continue;
        }

        workers.emplace_back([start, end, times, &results]() {
            for (std::size_t idx = start; idx < end; ++idx) {
                results[idx] = apply_fun_multiple_times(static_cast<uint64_t>(idx + 1), times);
            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started
    ).count();

    std::cout << "checksum=" << checksum(results) << "\n";
    std::cout << "elapsed_ms=" << elapsed_ms << "\n";
    return 0;
}
