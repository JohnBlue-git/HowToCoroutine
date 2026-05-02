# PrimaryCoroutineType

This folder demonstrates the primary C++20 coroutine return-type designs:

- **Eager Task**: begins execution immediately until the first `co_await`.
- **Lazy Task**: does nothing until the caller explicitly resumes or awaits it.
- **Fire-and-Forget**: starts and detaches, with no handle returned to the caller.
- **Generator**: produces a sequence of values with `co_yield`.
- **Signalling / Awaitable**: external events resume a suspended coroutine.

## Files

- `src/coroutine_types.hpp`: coroutine return-type implementations and awaitables.
- `src/main.cpp`: example usage for each pattern.

## Build

From repository root:

```bash
mkdir -p build
g++ -std=c++20 -O2 -pthread src/main.cpp -o build/primary_coroutine_types
```

## Run

```bash
./build/primary_coroutine_types
```

## Comparision among primary coroutine types

In C++20, the "design" of a coroutine is determined by its **Return Type** and the associated **Promise Type**. Because the language provides a low-level framework rather than a finished library, developers have designed several "patterns" to handle different asynchronous needs. Here are the primary kinds of coroutine designs:

### 1. Eager Task (The "Future/Promise" Style)
This is the most common design for general asynchronous work. When you call the function, it starts executing immediately until it hits its first `co_await`.

*   **Behavior:** Starts immediately.
*   **Use Case:** Offloading a specific calculation or I/O task where you eventually need the result.
*   **Example:** `Task<int> calculate_sum();`
*   **Analogy:** Ordering a pizza. You call, they start making it immediately, and you get a "receipt" (the Task object) to check when it's done.

### 2. Lazy Task (The "Cold" Coroutine)
In this design, the coroutine does **nothing** when called. It only starts executing when you explicitly `co_await` it or call `.resume()`.

*   **Behavior:** Suspends at the starting line (`initial_suspend` returns `suspend_always`).
*   **Use Case:** Complex pipelines or "Generators" where you want to define work now but execute it later to save resources.
*   **Example:** `Lazy<void> process_data();`

### 3. Fire-and-Forget (The "Detached" Style)
This is a coroutine that returns `void` (or a specific `detach` type). Once called, the caller has no way to track it, wait for it, or get a value back.

*   **Behavior:** Runs independently in the background.
*   **Use Case:** Logging, telemetry, or "event-like" triggers where the result doesn't affect the main flow.
*   **Example:** `fire_forget save_log(std::string msg);`
*   **Risk:** If the coroutine crashes or outlives the objects it references, it can cause memory corruption because the caller isn't waiting for it to finish.

### 4. Generators (The "Stream" Style)
Unlike Tasks that return one value at the end, a Generator produces a **sequence** of values over time using `co_yield`.

*   **Behavior:** Suspends every time it produces a value, then resumes when the caller asks for the next one.
*   **Use Case:** Reading lines from a file, generating an infinite mathematical sequence, or iterating over a tree.
*   **Example:** `Generator<int> range(int start, int end);`

### 5. Multi-Shot / Signalling Coroutines
These are specialized designs used in networking or UI frameworks. They might suspend and resume multiple times in response to external events.

| Kind | Return Type | Typical Usage |
| :--- | :--- | :--- |
| **Task / Awaitable** | `Task<T>` | Standard async I/O (like your file write). |
| **Fire-and-Forget** | `void` / `detached` | UI button clicks, "Send and ignore" logs. |
| **Generator** | `Generator<T>` | Data streaming and lazy sequences. |
| **C-Style Wrapper** | `CustomType` | Bridging coroutines to old C-style callback APIs. |

### Comparison: Eager vs. Lazy vs. Fire-and-Forget

| Feature | Task (Eager) | Task (Lazy) | Fire-and-Forget |
| :--- | :--- | :--- | :--- |
| **Starts on call?** | Yes | No | Yes |
| **Trackable?** | Yes (via handle) | Yes (via handle) | No |
| **Returns value?** | Yes | Yes | No |
| **Thread Safety** | Depends on Executor | Depends on Executor | High Risk |
