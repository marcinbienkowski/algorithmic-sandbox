# Testing

This document catalogs the test programs in `tests/programs/` and what each one exercises. See
[README.md](README.md) for how to run the test suite. Sections and ordering mirror the four
groups `tests/test_sandbox.py` itself is split into.

### Testing sandbox with wrong arguments

These tests exercise the `sandbox`/`sandbox_coverage` binary itself rather than a test program,
covering error paths outside the seccomp/ptrace machinery:

- `test_sandbox_no_arguments` - Invoked with no arguments; checks the usage message
- `test_sandbox_nonexistent_file` - Targets a nonexistent executable path
- `test_sandbox_nonexecutable_file` - Targets an empty, non-executable file
- `test_sandbox_internal_error_on_setrlimit` - Lowers the `RLIMIT_CPU` hard limit before the
  sandbox runs, so its own `setrlimit()` call fails
- `test_sandbox_internal_error_on_ptrace_traceme` - Runs the sandbox under `strace`, so
  `PTRACE_TRACEME` fails because the process is already traced
- `test_sandbox_internal_error_unwritable_result_file` - Points the result file at a nonexistent
  directory

### These programs should execute normally

- `prog_env_variables.cpp` - Environment variable access test
- `prog_file_access_allowed.cpp` - Tests legitimate file operations (reading allowed files)
- `prog_hello_world.cpp` - Basic functionality baseline test
- `prog_many_writes.cpp` - Tests high-frequency I/O operations (200k writes)
- `prog_memory_remap.cpp` - Tests memory remapping operations
- **Bubble Sort Programs** (`prog_bubble_sort.*`: C, C++, Rust)
- **Fibonacci Programs** (`prog_fibonacci.*`: C, C++, Rust)
- **Prime Numbers Programs** (`prog_prime_numbers.*`: C, C++, Rust)

### These programs should exit with some errors

- `prog_divide_by_zero.cpp` - Tests division by zero exception handling
- `prog_divide_by_zero_asm.cpp` - Assembly-level division by zero test
- `prog_infinite_loop.cpp` - Infinite loop to test CPU time limit enforcement (`SIGPROF`)
- `prog_int3.cpp` - A bare `int3` is a signal-delivery-stop
- `prog_large_local.cpp` - Tests large local variable allocation
- `prog_large_static.cpp` - Tests large static variable allocation
- `prog_malloc_bounded.cpp` - Controlled memory allocation within limits
- `prog_malloc_unbounded.cpp` - Infinite memory allocation loop to test memory limit enforcement
- `prog_recursive_fibonacci.cpp` - Tests recursive algorithm with high CPU usage
- `prog_ret2libc.cpp` - Return-to-libc attack simulation
- `prog_storm_fault.cpp` - The signal-delivery-stop counterpart to `prog_storm_mmap.cpp`: a
  `SIGSEGV` handler that returns
- `prog_storm_mmap.cpp` - Loops on `mmap()`, which keeps the tracee in a seccomp stop almost
  continuously; incurs a `SIGKILL` that lands on an already-reaped stop. `test_prog_storm_mmap_repeated`
  reruns it 10 times at a short time limit to raise the odds of hitting that race
- `prog_stack_overflow.cpp` - Recursive function to trigger stack overflow protection (`SIGSEGV`)
- `prog_stack_overflow.rs` - Rust equivalent of the above (Rust has a signal handler for it)
- `prog_vector_growth.cpp` - Tests dynamic memory growth patterns with STL vectors
- `prog_very_many_writes.cpp` - Like `prog_many_writes.cpp`, but with more I/O operations; checks
  timeout precision under heavy write load

### These programs should be blocked as they use forbidden syscalls

- `prog_execve.cpp` - Tests execve system call blocking
- `prog_execve_asm.cpp` - Assembly-level execve system call test
- `prog_file_access_forbidden.cpp` - Attempts to access restricted files
- `prog_fork.cpp` - Tests process creation blocking (clone syscall)
- `prog_getrlimit.cpp` - Tests resource limit introspection
- `prog_i386_abi.cpp` - Attempts to use 32-bit ABI
- `prog_inotify.cpp` - Tests file system monitoring blocking (inotify_init syscall)
- `prog_mmap_shared_stdout.cpp` - Tests whether the tracee can rewrite output already written
- `prog_open_wrong_address.cpp` - Tests invalid memory address handling
- `prog_open_wrong_flags.cpp` - Opens an allowlisted path with a disallowed flag (`O_RDWR`)
- `prog_pthread.cpp` - Tests threading operations; rejected at `rt_sigaction`, before the
  underlying `clone3` ever runs
- `prog_readlinkat_wrong_path.cpp` - Calls `readlinkat` on a path outside the allowlist
- `prog_self_sigabrt.cpp` - `raise(SIGABRT)`
- `prog_self_sigprof.cpp` - `raise(SIGPROF)`
- `prog_self_sigstop.cpp` - `raise(SIGSTOP)`
- `prog_sleep.cpp` - Tests nanosleep syscall blocking
- `prog_socket.cpp` - Tests network access blocking (socket syscall)
- `prog_system_call.cpp` - Tests `system()`, which forks internally; rejected at `rt_sigaction`
  like `prog_pthread.cpp`, before the underlying `clone3` ever runs
