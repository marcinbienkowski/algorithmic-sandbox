#ifndef SANDBOX_SYSCALL_HANDLER_H
#define SANDBOX_SYSCALL_HANDLER_H

#include "config.h"
#include "util.h"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <optional>
#include <ranges>
#include <span>
#include <sys/ptrace.h>

#include <linux/ptrace.h>


class SyscallHandler {
    Config::user_space_allowed_syscalls_t userspace_allowed_syscalls;
    std::set<std::string> openat_allowed_paths;
    std::set<std::string> readlinkat_allowed_paths;
    pid_t pid;
    ptrace_syscall_info syscall = {};

    [[nodiscard]] std::optional<uint64_t> read_word(uint64_t addr) const {
        errno = 0;
        const long data = ptrace(static_cast<__ptrace_request>(PTRACE_PEEKDATA), pid, static_cast<long>(addr), 0);
        if (errno == ESRCH or errno == EIO or errno == EFAULT)
            return std::nullopt;
        Util::ensure(errno == 0, "PTRACE_PEEKDATA");
        return static_cast<uint64_t>(data);
    }

    [[nodiscard]] std::optional<std::string> read_path(uint64_t path_addr) const {
        std::string path;
        path.reserve(PATH_MAX);
        for (size_t j = 0; j < PATH_MAX; j++) {
            const auto word = read_word(path_addr + j);
            if (not word)
                return std::nullopt;
            const char c = static_cast<char>(*word & 0xffU);
            if (c == 0)
                break;
            path.push_back(c);
        }
        return path;
    }

    [[nodiscard]] bool path_allowed(const std::set<std::string>& allowed_paths) const {
        const auto path = read_path(syscall.seccomp.args[1]);
        return path and allowed_paths.contains(*path);
    }

public:
    explicit SyscallHandler(pid_t _pid):
        userspace_allowed_syscalls(Config::get_user_space_allowed_syscalls()),
        openat_allowed_paths(Config::get_openat_allowed_paths()),
        readlinkat_allowed_paths(Config::get_readlinkat_allowed_paths()),
        pid(_pid) {}

    [[nodiscard]] int get_id() const { return static_cast<int>(syscall.seccomp.nr); }

    [[nodiscard]] bool read_syscall() {
        // SIGKILL (from our timeout watchdog, or other source) may arrive when tracee is in ptrace-stop.
        // SIGKILL will not generate signal-delivery-stop, so there will be no PTRACE_EVENT_EXIT.
        if (ptrace(static_cast<__ptrace_request>(PTRACE_GET_SYSCALL_INFO), pid, sizeof(syscall), std::bit_cast<long>(&syscall)) == -1) {
            Util::ensure(errno == ESRCH, "PTRACE_GET_SYSCALL_INFO");
            return false;
        }
        return syscall.op == PTRACE_SYSCALL_INFO_SECCOMP;
    }

    [[nodiscard]] std::string syscall_to_string() const {
        auto s = std::format("SYSCALL {} called; args = ", syscall.seccomp.nr);
        for (const auto& arg: std::span{syscall.seccomp.args})
              s += std::format("{} ", arg);
        return s;
    }  // LCOV_EXCL_LINE

    [[nodiscard]] bool is_allowed() const {
        if (syscall.seccomp.nr == SYS_openat and not path_allowed(openat_allowed_paths))
            return false;
        if (syscall.seccomp.nr == SYS_readlinkat and not path_allowed(readlinkat_allowed_paths))
            return false;
        const auto args_array = std::to_array(syscall.seccomp.args);
        const auto arg_matches = [&args_array](const Config::arg_mask_t& constraint) {
            const auto& [arg_id, mask, value] = constraint;
            return (args_array.at(arg_id) & mask) == value;
        };
        const auto [begin, end] = userspace_allowed_syscalls.equal_range(static_cast<uint64_t>(syscall.seccomp.nr));
        return std::ranges::any_of(std::ranges::subrange{begin, end}, [&arg_matches](const auto& entry) {
            return std::ranges::all_of(entry.second, arg_matches);
        });
    }
};

#endif
