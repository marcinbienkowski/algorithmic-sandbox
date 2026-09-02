#ifndef SANDBOX_SUBPROCESS_H
#define SANDBOX_SUBPROCESS_H

#include "config.h"
#include "parameters.h"
#include <array>
#include <cmath>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <utility>
#include <vector>


extern "C" void __gcov_dump(void);  // NOLINT
inline void flush_coverage() {
    #ifdef COVERAGE
    __gcov_dump();
    #endif
}  // LCOV_EXCL_LINE


class Subprocess {
    parameters params;

    static void enforce(bool condition, int sig) {
        if (not condition)
            raise(sig);  // NOLINT(cert-err33-c)  // LCOV_EXCL_LINE
    }

    static void limit (__rlimit_resource resource, rlim_t rlim, rlim_t rlim_max = 0) {
        const rlimit rl = { .rlim_cur = rlim, .rlim_max = std::max(rlim, rlim_max) };
        enforce(setrlimit(resource, &rl) == 0, SIGUSR1);
    }

    void set_important_limits() const {
        const rlim_t mem_limit_in_bytes = params.mem_limit_in_kb * 1024;
        // Failsafe if tracer-side watchdog fails: should never occur.
        const auto failsafe_stop_time = static_cast<rlim_t>(std::ceil(1.5 * static_cast<double>(params.time_limit_in_ms) / 1000.0)) + 3;
        limit(RLIMIT_CPU, failsafe_stop_time, failsafe_stop_time);
        limit(RLIMIT_AS, mem_limit_in_bytes);
        limit(RLIMIT_DATA, mem_limit_in_bytes);
        limit(RLIMIT_STACK, mem_limit_in_bytes);
        limit(RLIMIT_FSIZE, Config::max_output_size_in_mb * 1024 * 1024);
    }

    static void set_optional_limits() {
        limit(RLIMIT_CORE, 0);
        limit(RLIMIT_LOCKS, 0);
        limit(RLIMIT_MEMLOCK, 0);
        limit(RLIMIT_MSGQUEUE, 0);
        limit(RLIMIT_NICE, 0);
        limit(RLIMIT_NOFILE, Config::max_file_descriptors);
        limit(RLIMIT_NPROC, 1);
        limit(RLIMIT_RSS, RLIM_INFINITY);
        limit(RLIMIT_RTPRIO, 0);
        limit(RLIMIT_RTTIME, 0);
        limit(RLIMIT_SIGPENDING, Config::max_pending_signals);
    }

    [[nodiscard]] static bool install_bpf_filter() {
        std::vector<sock_filter> prog = {
            // Kill the process if its syscall arch != x86_64.
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, arch)),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS)
        };
        for (const auto& [nr, required_args]: Config::get_kernel_space_allowed_syscalls(getpid())) {
            // Allows if syscall and its argument match, else proceed to next block.
            const std::size_t total = 2 + required_args.size() * 4 + 1;
            const std::size_t block_start = prog.size();
            const auto reject_distance = [&] { return static_cast<unsigned char>(total - (prog.size() - block_start + 1)); };
            prog.push_back(BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)));
            prog.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, static_cast<uint32_t>(nr), 0, reject_distance()));
            for (const auto& [arg_id, value]: required_args) {
                const auto lo_offset = static_cast<uint32_t>(offsetof(seccomp_data, args) + arg_id * sizeof(uint64_t));
                prog.push_back(BPF_STMT(BPF_LD | BPF_W | BPF_ABS, lo_offset));
                prog.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, static_cast<uint32_t>(value), 0, reject_distance()));
                prog.push_back(BPF_STMT(BPF_LD | BPF_W | BPF_ABS, static_cast<uint32_t>(lo_offset + sizeof(uint32_t))));
                prog.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, static_cast<uint32_t>(value >> 32U), 0, reject_distance()));
            }
            prog.push_back(BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW));
        }
        prog.push_back(BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE));  // Anything else is decided by SyscallHandler.
        const sock_fprog program{.len = static_cast<unsigned short>(prog.size()), .filter = prog.data()};
        return prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0 and prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0;
    }

public:
    explicit Subprocess(parameters _params): params(std::move(_params)) {}

    void set_limits() const {
        set_important_limits();
        set_optional_limits();
    }

    void execute_program() const {
        enforce(ptrace(static_cast<__ptrace_request>(PTRACE_TRACEME), 0, 0, 0) != -1, SIGUSR2);
        enforce(raise(SIGSTOP) == 0, SIGUSR2);
        enforce(install_bpf_filter(), SIGUSR2);
        std::string binary_path = params.program_binary;
        const std::array<char*, 2> args = {binary_path.data(), nullptr};
        flush_coverage();
        enforce(execve(params.program_binary.c_str(), args.data(), nullptr) == 0, SIGKILL);  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_LINE
};

#endif
