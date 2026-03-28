#pragma once

#include <cstdint>

inline int compute_double(int x) {
    return x * 2;
}

inline long long compute_sum(int count) {
    long long total = 0;
    for (int i = 1; i <= count; ++i) {
        total += compute_double(i);
    }
    return total;
}
