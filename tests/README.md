# tests/test_resource_compare.py Guide

This document explains the code structure and test logic in [test_resource_compare.py](test_resource_compare.py).

## Purpose

This test compiles three C++ implementations, runs them with the same input parameters, and compares both resource usage and output correctness:

- fork version
- thread version
- coroutine version

Compared metrics include:

- cpu_seconds: child-process CPU time (user + system)
- max_rss_kb: maximum observed RSS during execution (KB)
- wall_seconds: real elapsed time
- elapsed_ms: execution time reported by the C++ program itself (milliseconds)
- checksum: result integrity value for consistency checks

## File Structure

### 1) _format_grid_table

Purpose: formats benchmark results into a plain-text table that is easier to read in pytest output.

Inputs:

- headers: list of column names
- rows: row data

Output:

- a border-style table string

### 2) build_dir fixture (session scope)

Purpose: compiles the three binaries once at the start of the test session.

Flow:

1. Locate the project root directory.
2. Create a temporary build directory.
3. Compile three source files with g++ -std=c++20 -O2 -pthread.
4. Return the build directory path for later fixtures/tests.

Benefits:

- avoids recompiling for every test
- reduces total test runtime

### 3) run_with_metrics fixture

Purpose: provides a reusable function that runs one implementation and collects metrics.

Call signature:

- _run(name, count, times)

Main steps:

1. Build command arguments and start the child process (subprocess.Popen).
2. Use resource.getrusage(resource.RUSAGE_CHILDREN) before/after to calculate child CPU usage.
3. Poll process RSS with psutil during execution and track peak memory usage.
4. Read stdout/stderr and fail immediately if the return code is non-zero.
5. Extract checksum and elapsed_ms from stdout using regular expressions.
6. Return a metrics dictionary.

Error handling:

- if the process fails, AssertionError includes return code, stdout, and stderr
- if checksum or elapsed_ms is missing in stdout, the test fails immediately to avoid silent errors

### 4) test_compare_cpu_and_memory_usage

Purpose: core integration test that compares all three implementations under identical conditions.

What it verifies:

1. Use fixed inputs: count=800000 and times=24.
2. Run fork, thread, and coroutine in sequence and collect results.
3. Assert all checksums are identical (correctness).
4. Assert cpu_seconds, max_rss_kb, and wall_seconds are valid positive values.
5. Print a comparison table for human inspection.

## How to Run

Run from the project root:

```bash
pytest -s -q
```

Notes:

- -s: keeps print output (so you can see the table)
- -q: quiet/concise pytest output

## Measurement Notes

- CPU time is computed from child-process usage deltas, not the parent process.
- Peak memory is measured via polling at 5ms intervals, so it is an approximate peak.
- wall_seconds is end-to-end elapsed time and is more sensitive to system load.
- The main goal is relative comparison across implementations in the same environment, not absolute performance guarantees.
