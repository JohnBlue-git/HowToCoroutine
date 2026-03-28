# Functional-Program-Style/src

This folder demonstrates two asynchronous programming styles in C++:

1. Thread/callback-based async (`traditional_async.cpp`) using `std::thread` and explicit callback joins.
2. Coroutine-based async (`coroutine_async.cpp`) using C++20 coroutines and a custom `task<T>` awaitable.

The shared utilities are in `common.hpp`.

## Files

- `common.hpp`
  - `compute_double(int x)`: pure function that returns `x * 2`.
  - `compute_sum(int count)`: iterative sum of `compute_double(1..count)`, not used by runtime examples but provided for direct expected-value comparison.

- `traditional_async.cpp`
  - `async_double(int x, std::function<void(int)> callback)`
    - launches a worker thread that computes `compute_double(x)` and calls `callback(result)`.
    - returns the `std::thread` object so the caller controls join semantics.

  - `async_sum(int count)`
    - reserves a `std::vector<std::thread>` for `count` worker threads.
    - uses `std::mutex` + `lock_guard` to safely accumulate `total` in callback from each worker.
    - starts `async_double(i, lambda)` for `i = 1..count` and then joins all threads.
    - returns completed sum.

  - `main(int argc, char **argv)`
    - parses `count` from `argv[1]` (default 100), handles `count <= 0` early exit.
    - measures runtime with `std::chrono::steady_clock`.
    - calls `async_sum(count)` and prints `result=` and `elapsed_ms=`.

- `coroutine_async.cpp`
  - `task<T>` (custom coroutine type)
    - wrapper around `std::coroutine_handle<promise_type>`.
    - movable, non-copyable; destructor destroys coroutine handle.
    - `ready()`, `get()` to run coroutine to completion and rethrow exceptions if present.
    - nested `awaiter` type with `await_ready`, `await_suspend`, `await_resume`.
    - `operator co_await()` returns awaiter, enabling `co_await` on tasks.

  - `promise_type` inside `task<T>`
    - stores `T value` and `std::exception_ptr exception`.
    - `get_return_object()`, `initial_suspend()`, `final_suspend()`, `return_value`, `unhandled_exception`.

  - `async_double(int x)` coroutine
    - `co_return compute_double(x)`.

  - `async_sum(int count)` coroutine
    - loops `i = 1..count`, `co_await async_double(i)`, accumulates into `total`, `co_return total`.

  - `main(int argc, char **argv)`
    - same parse + timing as traditional path.
    - calls `async_sum(count)` to obtain `task<int>` job.
    - calls `job.get()` to run coroutine and get final result, prints `result=` and `elapsed_ms=`.

## Build

```bash
g++ -std=c++20 -O2 -pthread traditional_async.cpp -o traditional
g++ -std=c++20 -O2 -pthread coroutine_async.cpp -o coroutine
```

## Run

- `./traditional 100`
- `./coroutine 100`

Expected output format:

```
result=<sum>
elapsed_ms=<milliseconds>
```

## Behavior notes

- Both styles compute the same logical sum: `2 + 4 + 6 + ... + 2*count`.
- `traditional_async` executes in parallel threads; `coroutine_async` executes sequentially in the current thread using coroutine suspension/resume (no thread pool).
- `async_sum` in `coroutine_async.cpp` demonstrates await sequencing and makes a direct comparison to the callback/locking complexity in `traditional_async.cpp`.

