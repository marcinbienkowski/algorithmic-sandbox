#ifndef SANDBOX_TRACER_H
#define SANDBOX_TRACER_H

#include "parameters.h"
#include "syscall.h"
#include <atomic>
#include <charconv>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/wait.h>


class Tracer {
    SyscallHandler syscall_handler;
    std::string result_file;
    bool debug_mode;
    pid_t pid;
    unsigned long time_limit_in_ms;

    static inline int status_fd = -1;  // /proc/<pid>/status of the tracee
    static_assert(std::atomic<bool>::is_always_lock_free and std::atomic<int>::is_always_lock_free);  // signal safety
    static inline std::atomic<bool> watchdog_fired = false;
    static inline std::atomic<int> watchdog_kill_errno = 0;
    static inline std::atomic<int> memory_usage_in_kb = -1;  // no reading yet

    // Async-signal-safe for the watchdog. Keeps the largest reading: VmPeak never shrinks.
    // Failed pread() or a zombie tracee (no VmPeak line) leaves the value unchanged.
    static void update_memory_usage() {
        std::array<char, 4096> buffer = {};
        const ssize_t length = pread(status_fd, buffer.data(), buffer.size(), 0);
        const std::string_view status(buffer.data(), length > 0 ? static_cast<size_t>(length) : 0);
        const size_t value_pos = status.find_first_of("0123456789", status.find("VmPeak:"));
        if (value_pos == std::string_view::npos)
            return;  // LCOV_EXCL_LINE (tracee already a zombie: only a race with the watchdog's kill)
        const std::string_view value = status.substr(value_pos);
        int vm_peak_in_kb = 0;
        std::from_chars(std::to_address(value.begin()), std::to_address(value.end()), vm_peak_in_kb);
        if (memory_usage_in_kb < vm_peak_in_kb)
            memory_usage_in_kb = vm_peak_in_kb;
    }

    static void handle_timeout(int /*signal*/, siginfo_t* info, void* /*context*/) {
        const int saved_errno = errno;  // errno of code interrupted by SIGPROF
        watchdog_fired = true;
        update_memory_usage();  // the SIGKILL below may skip PTRACE_EVENT_EXIT
        if (kill(info->si_value.sival_int, SIGKILL) < 0)
            watchdog_kill_errno = errno;
        errno = saved_errno;
    }

    void log(std::string_view s) const { if (debug_mode) std::cerr << std::format("TRACEE PID = {}: {}\n", pid, s); }

    void ptrace_ignore_tracee_death(int request, long data, std::string_view error_string) const {
        errno = 0;
        const long result = ptrace(static_cast<__ptrace_request>(request), pid, 0, data);
        Util::ensure(result >= 0 or errno == ESRCH, error_string);
    }

    enum class ptrace_event: std::uint8_t { signal, exit, seccomp };

    void finalize_tracee(std::pair<int, int> code_subcode, long time_usage, bool terminate_tracee, std::string_view description = "") {
        if (terminate_tracee) {
            log(std::format("Terminating tracee due to {}", description));
            update_memory_usage();
            Util::ensure(kill(pid, SIGKILL) == 0, "kill");  // a zombie until reaped, so no ESRCH
        }
        Util::ensure(memory_usage_in_kb >= 0, "no memory reading");
        std::ofstream file;
        file.exceptions(std::fstream::failbit | std::fstream::badbit);
        file.open(result_file, std::fstream::out);
        file << std::format("{} {} {} {}\n", code_subcode.first, code_subcode.second, time_usage, memory_usage_in_kb.load());
        file.close();
    }

    [[nodiscard]] static ptrace_event get_ptrace_event(int status) {
        Util::ensure(WIFSTOPPED(status), "WIFSTOPPED");
        switch (const unsigned bare_ptrace_event = static_cast<unsigned>(status) >> 16U) {
            case PTRACE_EVENT_EXIT: return ptrace_event::exit;
            case PTRACE_EVENT_SECCOMP: return ptrace_event::seccomp;
            default: Util::ensure(bare_ptrace_event == 0, "ptrace_event");
                return ptrace_event::signal;
        }
    }

    [[nodiscard]] std::pair<int, long> wait_for_tracee() const {
        int status = -1;
        rusage ru = {};
        const pid_t result = wait4(pid, &status, 0, &ru);   // Restarted if interrupted by SIGPROF
        Util::ensure(result == pid, "wait4");
        return {
            status,
            ((ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) * 1000) + ((ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1000)
        };
    }

    void let_tracee_continue(int signal) const {
        ptrace_ignore_tracee_death(PTRACE_CONT, signal, "PTRACE_CONT");
    }

    // Returns true if the syscall is allowed.
    bool process_syscall() {
        if (syscall_handler.read_syscall()) {
            log(syscall_handler.syscall_to_string());
            return syscall_handler.is_allowed();
        }
        log("Tracee was killed in seccomp stop");
        return true;
    }

    // Arms timer for the tracee
    void arm_timeout_timer() const {
        status_fd = open(std::format("/proc/{}/status", pid).c_str(), O_RDONLY | O_CLOEXEC);  // before the handler can run
        Util::ensure(status_fd >= 0, "open /proc/<pid>/status");

        struct sigaction sa = {};
        sa.sa_sigaction = &handle_timeout;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        Util::ensure(sigaction(SIGPROF, &sa, nullptr) == 0, "sigaction");

        clockid_t clock_id = {};
        sigevent sev = { .sigev_value = { .sival_int = pid }, .sigev_signo = SIGPROF, .sigev_notify = SIGEV_SIGNAL, ._sigev_un = {} };
        timer_t timer = nullptr;
        Util::ensure(clock_getcpuclockid(pid, &clock_id) == 0, "clock_getcpuclockid");
        Util::ensure(timer_create(clock_id, &sev, &timer) == 0, "timer_create");

        const itimerspec deadline = { .it_interval = {}, .it_value = {
            .tv_sec = static_cast<time_t>(time_limit_in_ms / 1000),
            .tv_nsec = static_cast<long>(time_limit_in_ms % 1000) * 1'000'000 } };
        Util::ensure(timer_settime(timer, TIMER_ABSTIME, &deadline, nullptr) == 0, "timer_settime");
        log(std::format("Armed CPU-time watchdog at {} ms", time_limit_in_ms));
    }

public:
    explicit Tracer(pid_t _pid, const parameters& params) :
                    syscall_handler(_pid, params.program_binary), result_file(params.result_file),
                    debug_mode(params.print_debug), pid(_pid), time_limit_in_ms(params.time_limit_in_ms) {}

    bool initialize() {
        // Stop due to tracee calling raise(SIGSTOP)
        const int sync_status = wait_for_tracee().first;
        if (WIFSIGNALED(sync_status)) {  // error during tracee initialization
            Util::ensure(WTERMSIG(sync_status) == SIGUSR1 or WTERMSIG(sync_status) == SIGUSR2, "WTERMSIG");
            if (WTERMSIG(sync_status) == SIGUSR1)
                Util::ensure(false, "setrlimit");
            else
                Util::ensure(false, "PTRACE_TRACEME or BPF filter install");
        }
        Util::ensure(WIFSTOPPED(sync_status) and WSTOPSIG(sync_status) == SIGSTOP, "initial sync");
        log("Tracee reached initial sync");
        ptrace_ignore_tracee_death(PTRACE_SETOPTIONS, PTRACE_O_TRACEEXIT | PTRACE_O_EXITKILL | PTRACE_O_TRACESECCOMP, "PTRACE_SETOPTIONS");
        let_tracee_continue(0);  // run till execve()

        // Ignore all syscalls before running actual program, including execve() call.
        auto [status, time_usage] = wait_for_tracee();
        while (get_ptrace_event(status) == ptrace_event::seccomp) {
            log("Bootstrap syscall");
            let_tracee_continue(0);
            std::tie(status, time_usage) = wait_for_tracee();
        }

        // Handling error when calling execve()
        if (get_ptrace_event(status) == ptrace_event::exit) {
            log("Tracee failed to execute execve()");
            memory_usage_in_kb = 0;
            let_tracee_continue(0);
            std::tie(status, time_usage) = wait_for_tracee();
            Util::ensure(WIFSIGNALED(status) and WTERMSIG(status) == SIGKILL, "WTERMSIG != SIGKILL");
            finalize_tracee({-1, 0}, time_usage, false);
            return false;
        }

        // Stop after a successful execve()
        const int stop_sig = WSTOPSIG(status);
        log(std::format("Initialization stopped by signal {}", stop_sig));
        Util::ensure(stop_sig == SIGTRAP or stop_sig == SIGSEGV, "stopping signal");
        arm_timeout_timer();
        let_tracee_continue(stop_sig == SIGTRAP ? 0 : stop_sig);  // this SIGTRAP is execve()'s stop, not the tracee's
        return true;
    }

    // returns true if tracing should continue
    bool process_event() {
        const auto [status, time_usage] = wait_for_tracee();
        // ESRCH only means the tracee had already died of its own accord before the timer expired.
        Util::ensure(watchdog_kill_errno == 0 or watchdog_kill_errno == ESRCH, "watchdog kill");

        if (WIFEXITED(status)) {
            log(std::format("Tracee terminated with exit status {}", WEXITSTATUS(status)));
            finalize_tracee({0, WEXITSTATUS(status)}, time_usage, false);
            return false;
        }

        if (WIFSIGNALED(status)) {
            const int term_sig = WTERMSIG(status);
            if (term_sig == SIGKILL) {
                Util::ensure(watchdog_fired, "SIGKILL");
                log("Tracee killed by the CPU-time watchdog");
                finalize_tracee({1, SIGPROF}, time_usage, false);
                return false;
            }
            log(std::format("Tracee killed by signal {}", term_sig));
            finalize_tracee({1, term_sig}, time_usage, false);
            return false;
        }

        switch (get_ptrace_event(status)) {
            case ptrace_event::exit:
                log("Tracee stopped right before exiting");
                update_memory_usage();
                let_tracee_continue(0);
                break;

            case ptrace_event::seccomp:
                if (not process_syscall()) {
                    const int syscall_id = syscall_handler.get_id();
                    finalize_tracee({2, syscall_id}, time_usage, true, std::format("disallowed syscall {}", syscall_id));
                    return false;
                }
                let_tracee_continue(0);
                break;

            case ptrace_event::signal: {
                const int signal = WSTOPSIG(status);
                log(std::format("Tracer catches signal {} and passes to tracee", signal));
                let_tracee_continue(signal);
                break;
            }
        }

        return true;
    }
};

#endif
