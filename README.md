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

### Repository Structure

This repository demonstrates C++ coroutines in two main contexts:

- **Fork-Thread-Coroutine**: A direct performance comparison of three parallel computation implementations: fork (processes), threads, and coroutines.
- **Functional-Program-Style**: Illustrates how coroutines improve functional programming by enabling cleaner async/await patterns compared to traditional callback-based approaches.

### Improvements in Functional Programming with Coroutines

Coroutines significantly enhance functional programming in C++ by allowing asynchronous operations to be expressed in a sequential, composable style using `co_await`. This addresses common pain points in functional async code:

- **Avoiding Callback Hell**: Traditional asynchronous programming relies on callbacks or futures, which can lead to deeply nested, hard-to-read code. Coroutines flatten this into linear, readable sequences.
- **Composability**: Async functions can be composed like regular functions, making it easier to build complex workflows without manual promise chaining.
- **Error Handling**: Exceptions work naturally across suspension points, unlike callback-based error propagation.
- **Readability**: Code resembles synchronous logic, improving maintainability and reducing bugs in functional-style async applications.

### Comparison Among Fork, Thread, and Coroutine

Here's a consolidated comparison of the three concurrency models used in this repository:

| Aspect | Fork | Thread | Coroutine |
|--------|------|--------|-----------|
| **Scheduling** | OS-preemptive (processes) | OS-preemptive | User-space cooperative |
| **Parallelism** | True (multi-core) | True (multi-core) | Single-threaded by default |
| **Overhead** | High (process creation, IPC) | Medium (context switches) | Low (lightweight switches) |
| **Isolation** | Strong (separate memory) | Weak (shared memory) | None (same process/thread) |
| **Scalability** | Limited by cores/processes | Limited by cores/threads | High (thousands of tasks) |
| **Best For** | Isolation-heavy tasks | CPU-bound parallelism | I/O-bound, many small tasks |
| **C++ Support** | `fork()`, pipes | `std::thread` | C++20 `<coroutine>` |

- **Fork**: Ideal for tasks requiring strong isolation (e.g., security-critical computations), but expensive for frequent use.
- **Thread**: Balances parallelism and ease; great for CPU-intensive work on multi-core systems.
- **Coroutine**: Excels in structuring complex async logic, especially in functional programming, but may need threading for full multi-core utilization.
