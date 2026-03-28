## Functional Program Style Project

This folder demonstrates C++ async/await patterns in two styles:
- `traditional_async.cpp`: callback-based, classic asynchronous style using `std::thread`.
- `coroutine_async.cpp`: modern C++20 coroutine-based style with `co_await`.

### Build and run

From repository root:

```bash
cd /workspaces/HowToCoroutine
mkdir -p build
# Build binaries
g++ -std=c++20 -O2 -pthread Functional-Program-Style/src/traditional_async.cpp -o build/traditional
g++ -std=c++20 -O2 -pthread Functional-Program-Style/src/coroutine_async.cpp -o build/coroutine

# Run samples
./build/traditional 100
./build/coroutine 100
```

Output example:

```
result=10100
elapsed_ms=3
```

## Philosophy

This example contrasts:
- explicit callback-based asynchronous execution (`traditional_async`) with a small thread pool style
- modern coroutine `task<>` combinator with `co_await` for sequential-style async flow
