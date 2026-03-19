# HowToCoroutine

## Coroutine Introduction

### Why use coroutine?

Coroutines are useful when you want to split work into many lightweight tasks with explicit suspension points, while avoiding the overhead of creating many OS threads or processes.

- Compared with threads:
	- Threads are preemptively scheduled by the OS and can run truly in parallel on multiple cores.
	- Coroutines are cooperatively scheduled in user space and are usually cheaper to create/switch.
	- Coroutines are often better for structuring many small tasks; threads are often better for raw CPU parallelism.

- Compared with fork:
	- `fork` gives strong process isolation, but process creation and IPC (pipes) are expensive.
	- Coroutines stay in one process and avoid IPC serialization overhead.
	- Coroutines are typically easier to scale to a very large number of logical tasks.

### Small-core vs large-core situations

In practice, the best model depends on CPU core count and workload type:

- Small core count (for example, 2-4 cores):
	- Thread and fork parallelism is limited by the small number of hardware cores.
	- Context-switch and synchronization overhead can become visible.
	- Coroutines can be attractive for reducing scheduling overhead and controlling execution order.

- Large core count (for example, 16+ cores):
	- Thread/fork versions can gain more wall-time speedup for CPU-bound workloads because they exploit more real parallelism.
	- A single-thread coroutine scheduler, like this demo, does not automatically use all cores.
	- Coroutines still help with structure and latency control, but multi-core speedup requires combining them with a thread pool or multi-thread runtime.

### Since which C++ version?

- Standardized coroutine support starts in C++20.
- This project uses `-std=c++20` and the `<coroutine>` header for that reason.

### What C++ does to support coroutines

C++ provides both language-level and library-level building blocks:

- Language keywords:
	- `co_await`: suspend until an awaitable is ready
	- `co_yield`: produce a value and suspend (generator-like patterns)
	- `co_return`: complete a coroutine

- Compiler transformation:
	- A coroutine function is transformed into a state machine.
	- Local state is stored in a coroutine frame across suspensions.

- Promise/handle model:
	- `promise_type` defines creation, suspension, return, and error behavior.
	- `std::coroutine_handle` provides low-level control (`resume`, `done`, `destroy`).

- Awaitable protocol:
	- Types can customize suspension behavior via awaiter methods (`await_ready`, `await_suspend`, `await_resume`).

Note: the standard gives primitives, but not a full scheduler runtime by default. Applications typically build or adopt a scheduler/executor on top.

## Sample Project

The following project demonstrates three C++ implementations for repeatedly applying:

- `f(x) = 2 * x`

Each implementation computes `f` multiple times per input value and stores final results in `main()`.

### Implementations

- `src/fork_version.cpp`: parallel via multiple child processes (`fork` + pipes)
- `src/thread_version.cpp`: parallel via `std::thread`
- `src/coroutine_version.cpp`: cooperative scheduling via C++20 coroutines

### Build Manually

```bash
g++ -std=c++20 -O2 -pthread src/fork_version.cpp -o fork
g++ -std=c++20 -O2 -pthread src/thread_version.cpp -o thread
g++ -std=c++20 -O2 -pthread src/coroutine_version.cpp -o coroutine
```

Run one version:

```bash
./thread 800000 24
```

Arguments:

1. `count`: number of input values, using `1..count`
2. `times`: how many times to apply `f(x) = 2*x`

### Pytest Benchmark Comparison

Test file: `tests/test_resource_compare.py`

It compiles all three binaries and compares:

- CPU time (`resource.getrusage`, user + system)
- peak memory (`psutil`, process RSS peak)
- wall-clock time (`time.perf_counter`)

### Field Meanings

| Field | Meaning |
| --- | --- |
| `version` | Which implementation is measured: `fork`, `thread`, or `coroutine`. |
| `cpu_seconds` | Total CPU time consumed by child process: user time + system time (seconds). |
| `max_rss_kb` | Peak resident memory usage in KB (maximum observed RSS). |
| `wall_seconds` | Real elapsed time from start to finish in seconds. |
| `elapsed_ms` | Runtime reported by each C++ program itself in milliseconds. |
| `checksum` | Result integrity value; all versions should produce the same checksum. |

Run:

```bash
pytest -s -q
```
