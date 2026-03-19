import re
import resource
import subprocess
import time
from pathlib import Path

import psutil
import pytest


def _format_grid_table(headers: list[str], rows: list[list[str]]) -> str:
    widths = [len(header) for header in headers]
    for row in rows:
        for idx, cell in enumerate(row):
            widths[idx] = max(widths[idx], len(cell))

    separator = "+" + "+".join("-" * (w + 2) for w in widths) + "+"
    header_line = "| " + " | ".join(headers[i].ljust(widths[i]) for i in range(len(headers))) + " |"

    body_lines = [
        "| " + " | ".join(row[i].ljust(widths[i]) for i in range(len(headers))) + " |"
        for row in rows
    ]

    return "\n".join([separator, header_line, separator, *body_lines, separator])


@pytest.fixture(scope="session")
def build_dir(tmp_path_factory: pytest.TempPathFactory) -> Path:
    root = Path(__file__).resolve().parent.parent
    out = tmp_path_factory.mktemp("build")

    sources = {
        "fork": root / "src" / "fork_version.cpp",
        "thread": root / "src" / "thread_version.cpp",
        "coroutine": root / "src" / "coroutine_version.cpp",
    }

    for name, src in sources.items():
        binary = out / name
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-O2",
                "-pthread",
                str(src),
                "-o",
                str(binary),
            ],
            check=True,
            cwd=root,
        )

    return out


@pytest.fixture()
def run_with_metrics(build_dir: Path):
    def _run(name: str, count: int, times: int) -> dict:
        binary = build_dir / name
        command = [str(binary), str(count), str(times)]

        before = resource.getrusage(resource.RUSAGE_CHILDREN)
        started = time.perf_counter()
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        monitor = psutil.Process(process.pid)
        peak_rss_bytes = 0

        while process.poll() is None:
            try:
                peak_rss_bytes = max(peak_rss_bytes, monitor.memory_info().rss)
            except psutil.Error:
                pass
            time.sleep(0.005)

        stdout, stderr = process.communicate()
        if process.returncode != 0:
            raise AssertionError(
                f"{name} failed with return code {process.returncode}\nstdout:\n{stdout}\nstderr:\n{stderr}"
            )

        finished = time.perf_counter()
        after = resource.getrusage(resource.RUSAGE_CHILDREN)

        checksum_match = re.search(r"checksum=(\d+)", stdout)
        if not checksum_match:
            raise AssertionError(f"checksum missing from stdout:\n{stdout}")

        elapsed_match = re.search(r"elapsed_ms=(\d+)", stdout)
        if not elapsed_match:
            raise AssertionError(f"elapsed_ms missing from stdout:\n{stdout}")

        cpu_seconds = (after.ru_utime - before.ru_utime) + (after.ru_stime - before.ru_stime)
        peak_rss_kb = int(peak_rss_bytes / 1024)

        return {
            "checksum": int(checksum_match.group(1)),
            "elapsed_ms": int(elapsed_match.group(1)),
            "cpu_seconds": max(cpu_seconds, 0.0),
            "max_rss_kb": max(peak_rss_kb, 1),
            "wall_seconds": max(finished - started, 0.0),
        }

    return _run


def test_compare_cpu_and_memory_usage(run_with_metrics):
    count = 800000
    times = 24
    versions = ["fork", "thread", "coroutine"]

    results = {name: run_with_metrics(name, count, times) for name in versions}

    checksums = {results[name]["checksum"] for name in versions}
    assert len(checksums) == 1, f"all versions must produce identical checksum: {results}"

    for name, metrics in results.items():
        assert metrics["cpu_seconds"] >= 0.0, f"cpu_seconds invalid for {name}: {metrics}"
        assert metrics["max_rss_kb"] > 0, f"max_rss_kb invalid for {name}: {metrics}"
        assert metrics["wall_seconds"] > 0.0, f"wall_seconds invalid for {name}: {metrics}"

    headers = ["version", "cpu_seconds", "max_rss_kb", "wall_seconds", "elapsed_ms", "checksum"]
    rows = [
        [
            name,
            f"{results[name]['cpu_seconds']:.6f}",
            str(results[name]["max_rss_kb"]),
            f"{results[name]['wall_seconds']:.6f}",
            str(results[name]["elapsed_ms"]),
            str(results[name]["checksum"]),
        ]
        for name in versions
    ]
    print("\n" + _format_grid_table(headers, rows))
