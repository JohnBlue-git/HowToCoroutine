#ifndef FORK_THREAD_COROUTINE_COMMON_HPP
#define FORK_THREAD_COROUTINE_COMMON_HPP

#include <cstdint>
#include <vector>

inline uint64_t apply_fun_multiple_times(uint64_t x, uint32_t times) {
    for (uint32_t i = 0; i < times; ++i) {
        x *= 2;
    }
    return x;
}

inline uint64_t checksum(const std::vector<uint64_t>& values) {
    uint64_t sum = 0;
    for (uint64_t value : values) {
        sum ^= value + 0x9e3779b97f4a7c15ULL + (sum << 6U) + (sum >> 2U);
    }
    return sum;
}

#endif // FORK_THREAD_COROUTINE_COMMON_HPP
