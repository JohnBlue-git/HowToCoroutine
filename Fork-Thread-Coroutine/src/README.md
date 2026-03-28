# Source Code Deep Dive

This document explains each C++ implementation in [fork_version.cpp](fork_version.cpp), [thread_version.cpp](thread_version.cpp), and [coroutine_version.cpp](coroutine_version.cpp) in detail.

## Common Goal Across All Versions

All three programs compute the same mathematical workload:

- Input sequence: integers from 1 to count
- Function: `f(x) = 2 * x`
- Repeat `times` times per input
- Store final values in a result array
- Print two fields:
  - `checksum=<value>`
  - `elapsed_ms=<value>`

Each version uses a different concurrency model, but preserves equivalent output semantics.

## Shared Building Blocks

A common helper header now centralizes shared behavior in `common.hpp` and is included by each source file (`fork_version.cpp`, `thread_version.cpp`, `coroutine_version.cpp`).

- `apply_fun_multiple_times(x, times)`
  - Loops `times` times and doubles `x` each iteration.
  - Equivalent to multiplying by `2^times`, but implemented iteratively to represent repeated work.

- `checksum(values)`
  - Computes a rolling hash-like checksum from the full result array.
  - Used as a correctness signal so different implementations can be compared safely.

This keeps each execution model source file focused on concurrency details instead of duplicating core math logic.

## [thread_version.cpp](thread_version.cpp)

### Execution Model

This version uses native OS threads via `std::thread`.

- Worker count is based on `std::thread::hardware_concurrency()`.
- If the platform reports 0, it falls back to 4 workers.
- Worker count is capped at `count` to avoid creating idle workers.

### Work Partitioning

The input range `[0, count)` is split into chunks:

- `chunk = ceil(count / worker_count)` implemented as `(count + worker_count - 1) / worker_count`.
- Worker `i` handles `[i * chunk, min(count, i * chunk + chunk))`.

This gives near-even distribution with simple index math.

### Data Ownership and Safety

- `results` is a shared vector sized to `count`.
- Every thread writes to a disjoint index range.
- No lock is needed because there is no overlapping write region.

### Lifecycle

1. Spawn one thread per active chunk.
2. Each thread computes values directly into `results[idx]`.
3. Main thread calls `join()` on all workers.
4. Compute and print checksum and elapsed time.

### Characteristics

- Pros:
  - Straightforward parallelism.
  - Minimal overhead compared to process-based design.
- Cons:
  - Uses real OS threads; scheduling overhead grows with thread count.
  - Shared-memory bugs are possible in more complex code (though avoided here).

## [fork_version.cpp](fork_version.cpp)

### Execution Model

This version uses multiple child processes created by `fork()` and communicates through pipes.

### Why Extra Helpers Exist

Two helpers are specific to reliable pipe I/O:

- `write_all(fd, buffer, bytes)`
  - Repeats `write()` until all bytes are sent.
  - Handles partial writes.

- `read_all(fd, buffer, bytes)`
  - Repeats `read()` until all bytes are received.
  - Detects unexpected EOF and read failures.

This is required because pipe reads/writes are not guaranteed to transfer the full buffer in one call.

### `WorkerPipe` Metadata

The parent stores per-child metadata:

- `pid`: child process ID
- `read_fd`: parent-side read end of pipe
- `start`: starting result index for this child
- `len`: number of result values expected from this child

This mapping lets the parent place each child's bytes into the correct slice of `results`.

### Parent/Child Flow

For each chunk:

1. Create a pipe.
2. Call `fork()`.
3. Child branch:
  - Close read end.
  - Compute local vector for its chunk.
  - Send raw bytes to parent using `write_all`.
  - Exit with `_exit(0)` (or `_exit(2)` on exception).
4. Parent branch:
  - Close write end.
  - Save worker metadata.

After launching all children, parent:

1. Reads each child payload into the corresponding segment of `results`.
2. Closes read descriptors.
3. Waits each child with `waitpid` and checks exit status.
4. Prints checksum and elapsed time.

### Error Handling

- `pipe`, `fork`, `waitpid` failures print system errors and return non-zero.
- I/O exceptions during pipe read are reported to stderr.
- Non-zero child exit status fails the run.

### Characteristics

- Pros:
  - Process isolation can be safer for fault containment.
  - Good demonstration of IPC mechanics.
- Cons:
  - Higher overhead: fork cost + inter-process data transfer.
  - More complex resource management (FD closing, wait status, EOF rules).

## [coroutine_version.cpp](coroutine_version.cpp)

### Execution Model

This version uses C++20 coroutines with a custom cooperative scheduler.

Unlike threads/processes, this is not parallel by default: all coroutine execution happens on one thread, and switching is explicit via suspension points.

### Core Types

#### `Task`

`Task` wraps a coroutine handle and manages ownership.

- `promise_type::get_return_object()` creates the `Task` from the promise.
- `initial_suspend()` returns `suspend_always`:
  - New coroutines start suspended and must be scheduled explicitly.
- `final_suspend()` also returns `suspend_always`:
  - Scheduler can observe completion and destroy handle safely.
- Destructor destroys the handle if still owned.
- `release()` transfers raw handle ownership to scheduler queue.

#### `Scheduler`

A simple round-robin scheduler over `std::deque<std::coroutine_handle<>>`.

- `schedule(handle)` pushes a coroutine to the queue.
- `run()` loop:
  1. Pop front handle.
  2. `resume()` it.
  3. If done, destroy it.
  4. Otherwise push it back.

This creates cooperative time-slicing behavior.

### `worker(...)` Coroutine

Each worker handles one chunk and writes into `results`.

- After every `yield_every` items, it executes `co_await std::suspend_always{}`.
- That suspension returns control to the scheduler, enabling fairness among workers.
- In `main`, `yield_every` is set to 1024.

### Main Flow

1. Choose worker count and chunk size exactly like other versions.
2. Create one coroutine worker per active chunk.
3. Move each coroutine handle into scheduler via `release()`.
4. Run scheduler until queue is empty.
5. Print checksum and elapsed time.

### Characteristics

- Pros:
  - Very lightweight context switches (user-space scheduling).
  - Clear illustration of cooperative multitasking concepts.
- Cons:
  - Single-threaded scheduler here means no true CPU parallelism.
  - Fairness/latency depends on where suspension points are placed.

## Side-by-Side Comparison

- `thread_version.cpp`
  - Model: preemptive multithreading (OS-managed)
  - Parallelism: yes
  - Communication: shared memory (disjoint slices)
  - Typical overhead: medium

- `fork_version.cpp`
  - Model: multi-process with IPC
  - Parallelism: yes
  - Communication: pipes and byte transfer
  - Typical overhead: highest

- `coroutine_version.cpp`
  - Model: cooperative scheduling in user space
  - Parallelism: no (in current single-thread scheduler)
  - Communication: shared memory in one process/thread
  - Typical overhead: low switching cost, but no multi-core speedup

## Complexity Notes

Let `N = count`, `T = times`.

- Compute work in all versions is `O(N * T)`.
- Additional overhead differs:
  - Thread: thread creation/join and scheduler overhead
  - Fork: process creation + IPC copy costs
  - Coroutine: queue operations + suspend/resume points

## Output Contract (Important for Tests)

All implementations are expected to print:

1. `checksum=<unsigned integer>`
2. `elapsed_ms=<unsigned integer>`

The test suite in [../tests/test_resource_compare.py](../tests/test_resource_compare.py) relies on that exact field format to parse and compare results.
