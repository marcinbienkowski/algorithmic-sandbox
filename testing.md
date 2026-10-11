# Testing

This document catalogs the test programs in `tests/programs/` and what each one exercises. See [README.md](README.md) for how to run the test suite. Sections and ordering mirror the four groups `tests/test_sandbox.py` itself is split into.

### Testing sandbox with wrong arguments

These tests exercise the `sandbox`/`sandbox_coverage` binary itself rather than a test program, covering error paths outside the seccomp/ptrace machinery:

- `test_sandbox_no_arguments` - Invoked with no arguments; checks the usage message
- `test_sandbox_nonexistent_file` - Targets a nonexistent executable path
- `test_sandbox_nonexecutable_file` - Targets an empty, non-executable file
- `test_sandbox_internal_error_on_setrlimit` - Lowers the `RLIMIT_CPU` hard limit before the sandbox runs, so its own `setrlimit()` call fails
- `test_sandbox_internal_error_on_ptrace_traceme` - Runs the sandbox under `strace`, so `PTRACE_TRACEME` fails because the process is already traced
- `test_sandbox_internal_error_unwritable_result_file` - Points the result file at a nonexistent directory
- `test_sandbox_closes_inherited_fds` - Runs a Rust program with an inherited fd 3, Rust would lseek in fd 4 then

### These programs should execute normally

- `prog_env_variables.cpp` - Prints its environment, which must be empty
- `prog_file_access_allowed.cpp` - Opens every path on the `openat` allowlist
- `prog_hash_set.rs` - Rust `HashSet`, whose random hasher seed needs `getrandom(GRND_INSECURE)`
- `prog_hello_world.cpp` - Basic functionality baseline test
- `prog_many_writes.cpp` - Tests high-frequency I/O operations (200k writes)
- `prog_memory_remap.cpp` - `realloc()` of a large block, which glibc implements with `mremap`
- **Bubble Sort Programs** (`prog_bubble_sort.*`: C, C++, Rust)
- **Fibonacci Programs** (`prog_fibonacci.*`: C, C++, Rust)
- **Prime Numbers Programs** (`prog_prime_numbers.*`: C, C++, Rust)

### These programs should exit with some errors

- `prog_divide_by_zero.cpp` - Division by a constant zero is undefined behaviour, which gcc compiles to `ud2`
- `prog_divide_by_zero_asm.cpp` - A real division by zero in assembly (`SIGFPE`)
- `prog_infinite_loop.cpp` - Infinite loop to test CPU time limit enforcement (`SIGPROF`)
- `prog_int3.cpp` - A bare `int3` is a signal-delivery-stop
- `prog_large_local.cpp` - A 40 MB local array overflows the stack (`SIGSEGV`)
- `prog_large_static.cpp` - A 40 MB static array exceeds `RLIMIT_AS`
- `prog_malloc_bounded.cpp` - Controlled memory allocation within limits
- `prog_malloc_unbounded.cpp` - Infinite memory allocation loop to test memory limit enforcement
- `prog_storm_fault.cpp` - The signal-delivery-stop counterpart to `prog_storm_mmap.cpp`: a `SIGSEGV` handler that returns
- `prog_storm_mmap.cpp` - Loops on `mmap()`, which keeps the tracee in a seccomp stop almost continuously
- `prog_stack_overflow.cpp` - Recursive function to trigger stack overflow protection (`SIGSEGV`)
- `prog_stack_overflow.rs` - Rust equivalent of the above (Rust has a signal handler for it)
- `prog_vector_growth.cpp` - Grows a `std::vector` until allocation fails
- `prog_very_many_writes.cpp` - Like `prog_many_writes.cpp`, but with more I/O operations; checks timeout precision under heavy write load

### These programs should be blocked as they use forbidden syscalls

- `prog_clone_raw.cpp` - A raw `clone` syscall
- `prog_execve.cpp` - Tests execve system call blocking
- `prog_execve_asm.cpp` - Assembly-level execve system call test
- `prog_file_access_forbidden.cpp` - Attempts to access restricted files
- `prog_fork.cpp` - `fork()`; rejected at `rt_sigprocmask` on glibc >= 2.41, at `clone` on older glibc
- `prog_getrlimit.cpp` - Queries resource limits; the `RLIMIT_STACK` query is allowed
- `prog_i386_abi.cpp` - Attempts to use 32-bit ABI
- `prog_inotify.cpp` - Tests file system monitoring blocking (inotify_init syscall)
- `prog_lseek_stdout.cpp` - Seeks in stdout, which would let the tracee rewrite output it has already written
- `prog_mmap_shared_stdout.cpp` - Tests whether the tracee can rewrite output already written
- `prog_open_wrong_address.cpp` - Tests invalid memory address handling
- `prog_open_wrong_flags.cpp` - Opens an allowlisted path with a disallowed flag (`O_RDWR`)
- `prog_pthread.cpp` - `pthread_create()`; rejected at `rt_sigaction`, before the underlying `clone3` ever runs
- `prog_readlinkat_wrong_path.cpp` - Calls `readlinkat` on a path outside the allowlist
- `prog_self_sigabrt.cpp` - `raise(SIGABRT)`
- `prog_self_sigprof.cpp` - `raise(SIGPROF)`
- `prog_self_sigstop.cpp` - `raise(SIGSTOP)`
- `prog_sigfpe_handler.cpp` - Installs a `SIGFPE` handler: harmless, but no algorithmic solution needs it
- `prog_sleep.cpp` - Tests nanosleep syscall blocking
- `prog_socket.cpp` - Tests network access blocking (socket syscall)
- `prog_system_call.cpp` - Tests `system()`, which forks internally; rejected at `rt_sigaction` like `prog_pthread.cpp`
