#ifndef SANDBOX_CONFIG_H
#define SANDBOX_CONFIG_H

#include <asm/prctl.h>
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <limits>
#include <linux/futex.h>
#include <map>
#include <set>
#include <string>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>


namespace Config {
    using arg_t = std::pair<unsigned, uint64_t>;                  // arg_id, required value
    using arg_mask_t = std::tuple<unsigned, uint64_t, uint64_t>;  // arg_id, mask, required value
    using kernel_space_allowed_syscalls_t = std::vector<std::pair<uint64_t, std::vector<arg_t>>>;
    using user_space_allowed_syscalls_t = std::multimap<uint64_t, std::vector<arg_mask_t>>;

    constexpr long long max_output_size_in_mb = 1000;
    constexpr int max_file_descriptors = 1024;
    constexpr int max_pending_signals = 1024;

    // Syscalls that can be decided in kernel space. Exact-equality arg checks only.
    inline kernel_space_allowed_syscalls_t get_kernel_space_allowed_syscalls(int pid) {
        const auto pid64 = static_cast<uint64_t>(pid);
        return {  // LCOV_EXCL_START
            {SYS_arch_prctl, {{0, ARCH_SET_FS}}},
            {SYS_close, {}},
            {SYS_exit_group, {}},
            {SYS_fstat, {}},
            {SYS_futex, {{1, FUTEX_WAKE_PRIVATE}}},
            {SYS_getpid, {}},
            {SYS_getrandom, {{2, GRND_NONBLOCK}}},
            {SYS_gettid, {}},
            {SYS_lseek, {{0, STDIN_FILENO}}},
            {SYS_munmap, {}},
            {SYS_poll, {{2, 0}}},
            {SYS_prlimit64, {{0, 0}, {1, RLIMIT_STACK}, {2, 0}}},
            {SYS_read, {}},
            {SYS_rseq, {}},
            {SYS_rt_sigaction, {{0, SIGPIPE}}},
            {SYS_rt_sigaction, {{0, SIGSEGV}}},
            {SYS_rt_sigaction, {{0, SIGBUS}}},
            {SYS_rt_sigprocmask, {{0, SIG_UNBLOCK}}},
            {SYS_rt_sigreturn, {}},
            {SYS_sigaltstack, {}},
            {SYS_sched_getaffinity, {{0, pid64}}},
            {SYS_set_robust_list, {}},
            {SYS_set_tid_address, {}},
            {SYS_tgkill, {{0, pid64}, {1, pid64}, {2, SIGABRT}}},
            {SYS_write, {{0, STDOUT_FILENO}}},
            {SYS_write, {{0, STDERR_FILENO}}},
            {SYS_writev, {{0, STDOUT_FILENO}}},
            {SYS_writev, {{0, STDERR_FILENO}}}
        };  // LCOV_EXCL_STOP
    }

    // Syscalls decided in user space: either need a bitmask, path verification, or memory usage verified after them.
    inline user_space_allowed_syscalls_t get_user_space_allowed_syscalls() {
        constexpr uint64_t no_mask = std::numeric_limits<uint64_t>::max();
        return {  // LCOV_EXCL_START
            {SYS_brk, {}},
            {SYS_mmap, {{2, PROT_EXEC, 0}, {4, no_mask, 0xFFFFFFFF}}},
            {SYS_mprotect, {{2, PROT_EXEC, 0}}},
            {SYS_mremap, {}},
            {SYS_openat, {{2, no_mask, O_RDONLY}}},
            {SYS_openat, {{2, no_mask, O_RDONLY | O_CLOEXEC}}},
            {SYS_readlinkat, {}}
        };  // LCOV_EXCL_STOP
    }

    // Paths program may open using SYS_openat
    inline std::set<std::string> get_openat_allowed_paths() {
        return {"/etc/timezone", "/dev/urandom", "/proc/self/maps", "/usr/share/zoneinfo/Europe/Warsaw"};
    }

    // Paths program may read using SYS_readlinkat
    inline std::set<std::string> get_readlinkat_allowed_paths(const std::string& binary_name) {
        return {binary_name, "/proc/self/exe"};
    }

} // namespace Config

#endif
