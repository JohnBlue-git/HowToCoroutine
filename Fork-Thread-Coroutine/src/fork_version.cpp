#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include "common.hpp"

namespace {

struct WorkerPipe {
    pid_t pid;
    int read_fd;
    std::size_t start;
    std::size_t len;
};

void write_all(int fd, const uint8_t* buffer, std::size_t bytes) {
    std::size_t written = 0;
    while (written < bytes) {
        ssize_t n = write(fd, buffer + written, bytes - written);
        if (n < 0) {
            throw std::runtime_error("write failed");
        }
        written += static_cast<std::size_t>(n);
    }
}

void read_all(int fd, uint8_t* buffer, std::size_t bytes) {
    std::size_t received = 0;
    while (received < bytes) {
        ssize_t n = read(fd, buffer + received, bytes - received);
        if (n < 0) {
            throw std::runtime_error("read failed");
        }
        if (n == 0) {
            throw std::runtime_error("unexpected EOF while reading child pipe");
        }
        received += static_cast<std::size_t>(n);
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::size_t count = (argc > 1) ? static_cast<std::size_t>(std::stoull(argv[1])) : 1000000ULL;
    const uint32_t times = (argc > 2) ? static_cast<uint32_t>(std::stoul(argv[2])) : 20U;

    if (count == 0) {
        std::cout << "checksum=0\n";
        return 0;
    }

    unsigned int worker_count = std::thread::hardware_concurrency();
    if (worker_count == 0) {
        worker_count = 4;
    }
    worker_count = std::min<unsigned int>(worker_count, static_cast<unsigned int>(count));

    const std::size_t chunk = (count + worker_count - 1) / worker_count;
    std::vector<uint64_t> results(count, 0);
    std::vector<WorkerPipe> workers;
    workers.reserve(worker_count);

    auto started = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < worker_count; ++i) {
        const std::size_t start = i * chunk;
        const std::size_t end = std::min(count, start + chunk);
        if (start >= end) {
            continue;
        }

        int pipe_fd[2];
        if (pipe(pipe_fd) != 0) {
            std::perror("pipe");
            return 1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            std::perror("fork");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            return 1;
        }

        if (pid == 0) {
            close(pipe_fd[0]);
            try {
                std::vector<uint64_t> local;
                local.reserve(end - start);
                for (std::size_t idx = start; idx < end; ++idx) {
                    local.push_back(apply_fun_multiple_times(static_cast<uint64_t>(idx + 1), times));
                }
                write_all(
                    pipe_fd[1],
                    reinterpret_cast<const uint8_t*>(local.data()),
                    local.size() * sizeof(uint64_t)
                );
                close(pipe_fd[1]);
                _exit(0);
            } catch (...) {
                close(pipe_fd[1]);
                _exit(2);
            }
        }

        close(pipe_fd[1]);
        workers.push_back(WorkerPipe{pid, pipe_fd[0], start, end - start});
    }

    for (const WorkerPipe& worker : workers) {
        auto* target = reinterpret_cast<uint8_t*>(results.data() + worker.start);
        const std::size_t bytes = worker.len * sizeof(uint64_t);
        try {
            read_all(worker.read_fd, target, bytes);
        } catch (const std::exception& ex) {
            std::cerr << "pipe read error: " << ex.what() << "\n";
            close(worker.read_fd);
            return 1;
        }
        close(worker.read_fd);
    }

    for (const WorkerPipe& worker : workers) {
        int status = 0;
        if (waitpid(worker.pid, &status, 0) < 0) {
            std::perror("waitpid");
            return 1;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            std::cerr << "child process failed for pid=" << worker.pid << "\n";
            return 1;
        }
    }

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started
    ).count();

    std::cout << "checksum=" << checksum(results) << "\n";
    std::cout << "elapsed_ms=" << elapsed_ms << "\n";
    return 0;
}
