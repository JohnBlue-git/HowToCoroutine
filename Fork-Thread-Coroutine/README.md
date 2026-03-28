## Comparision Project

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
