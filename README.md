# Algorithmic Sandbox

[![Local Tests](https://github.com/marcinbienkowski/algorithmic-sandbox/actions/workflows/tests.yml/badge.svg?branch=main)](https://github.com/marcinbienkowski/algorithmic-sandbox/actions/workflows/tests.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.txt)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](CMakeLists.txt)
[![Platform](https://img.shields.io/badge/platform-Linux%20x86__64-lightgrey.svg)](#requirements)

A secure sandbox for running submissions to algorithmic-problem competitions (Codeforces-style, as on most online judges). Such a submission is untrusted binary code (referred to as "tracee" below) that reads a test case from `stdin` and writes its answer to `stdout`. The sandbox executes it under strict security constraints and prevents malicious or resource-intensive tracees from affecting the host system.

The sandbox has been in production use since 2008. It runs student submissions in the Algorithms and Data Structures lab at the Institute of Computer Science, University of Wrocław, and it served the programming contests of the [Wrocław Computer Science Portal](https://web.archive.org/web/20240222064610/http://www.informatyka.wroc.pl/) while that site existed. It has executed more than 100,000 real problem solutions. It is not tied to these settings and can run any untrusted computational program.

It enforces memory and time limits. It uses a seccomp-BPF filter with `ptrace()` to monitor and restrict system calls, allowing only a safe subset. It allows tracees that perform computations and use `stdin`/`stdout`/`stderr`, and blocks everything else. In particular, it forbids opening files (apart from a few fixed system paths, read-only), network communication and process creation.

The goal is to block malicious tracees while allowing legitimate tracees to run. The sandbox is designed to err on the side of caution:
* A malicious tracee will *always* be blocked, assuming the kernel implements seccomp and `ptrace()` correctly.
* A legitimate tracee will *usually* run uninterrupted. This holds for computational tracees written in C/C++/Rust (see example programs in `tests/programs`) that have been statically compiled. (Static compilation avoids the need for opening shared libraries at runtime, which would be blocked.)

The allowlist is deliberately minimal. It admits only the syscalls that a computational tracee needs, including those the language runtime calls at startup and when reporting a crash. Being harmless is not enough for a syscall to be allowed. For example, a tracee that installs its own `SIGFPE` handler is stopped with `code = 2`, although catching `SIGFPE` does not help it bypass any limit.


## How it works

At startup, the sandbox closes all inherited file descriptors except `stdin`, `stdout` and `stderr`, and then forks. The child sets resource limits, requests tracing with `PTRACE_TRACEME`, installs a seccomp-BPF filter and executes the tracee binary.

The filter allows a fixed set of syscalls directly in the kernel, with exact-value checks on selected arguments. Any other syscall stops the tracee, and the sandbox inspects it in user space. Only anonymous `mmap` and `mprotect` without `PROT_EXEC`, read-only `openat` on a short list of paths, and `readlinkat` on the tracee's own binary pass this check. The tracee cannot create threads or processes, so its memory cannot change while the sandbox inspects syscall arguments.

Memory is capped with `RLIMIT_AS`. Time is enforced by a watchdog timer on the tracee's CPU-time clock, with `RLIMIT_CPU` as a failsafe. With `PTRACE_O_EXITKILL`, the kernel kills the tracee whenever the sandbox exits.

Unlike tools such as [isolate](https://github.com/ioi/isolate), the sandbox needs no root privileges, namespaces or cgroups.


## Requirements

* Linux x86_64 with kernel 5.9 or newer.
* glibc 2.34 or newer.
* A C++20 compiler, e.g., GCC 13 or newer.
* CMake.


## Usage

### Building
```bash
$> cmake .
$> make
```

### Running

```bash
$> ./sandbox <mem_limit_in_kb> <time_limit_in_ms> <program_binary> <result_file> <print_debug(true/false)>
```

### Examples

```bash
$> g++ -O2 -static -o prog_hello_world tests/programs/prog_hello_world.cpp
$> ./sandbox 4096 1000 ./prog_hello_world result-file.txt false
$> cat result-file.txt
```

A typical online-judge tracee reads a test case from `stdin` and writes its answer to `stdout`. To run it against a test case, redirect the sandbox's own `stdin`/`stdout`, which the tracee inherits (see [Input and output](#input-and-output) below):

```bash
$> g++ -O2 -static -o prog_bubble_sort tests/programs/prog_bubble_sort.cpp
$> ./sandbox 4096 1000 ./prog_bubble_sort result-file.txt false < tests/in_out/numbers.in > answer.out
$> cat answer.out
```

### Returned values

If sandboxing was successful, the sandbox returns `0` and writes four numbers to `<result_file>`:
* `code`:
  * `0` if the tracee exited normally;
  * `1` if the tracee was killed by a signal;
  * `2` if the tracee was aborted by the sandbox due to an illegal system call;
  * `-1` if the sandbox failed to start the tracee (for example, when the binary does not exist or is not executable).
* `subcode`: exit status if `code = 0`, delivered signal if `code = 1` and syscall number if `code = 2`.
* `time_usage` of the tracee in milliseconds.
* `memory_usage` of the tracee in kilobytes, equal to the peak virtual memory allocation.

If an internal error occurs, the sandbox returns `1` and `<result_file>` contains a description of the error. Even in such cases, the tracee is executed safely, although it may terminate prematurely.

The value of `memory_usage` never exceeds `memory_limit`.

If the tracee's actual runtime (user time + system time) exceeds `time_limit`, the sandbox terminates the tracee and returns `code = 1` with `subcode = 27 (SIGPROF)`. The timeout is enforced by a watchdog running in the *sandbox process*, armed on the tracee's own CPU-time clock. The tracee cannot evade the timeout.

If `print_debug` is equal to the string `true`, the sandbox logs to `stderr` the tracee's events and the syscalls that the seccomp filter passes to the sandbox for inspection.

### Input and output

The sandboxed tracee inherits `stdin`, `stdout`, and `stderr` of the sandbox process. You can redirect them as needed. The sandbox closes every other file descriptor it inherited, so the tracee cannot access them. The sandbox itself never reads anything from `stdin`, never writes anything to `stdout`, and writes to `stderr` only if `print_debug` is set to `true`.

Redirect `stdin` from a regular file, and have `stdout`/`stderr` go to a regular file or a pipe that is actively drained. The sandbox enforces only CPU-time limits and has no wall-clock limit. Thus, a blocking `read` on a terminal or an empty pipe, or a `write` to a full, undrained pipe, hangs the tracee forever without tripping the timeout. Do not use `/dev/null` either. When glibc first uses a stream attached to a character device, it calls `ioctl` to check whether the device is a terminal. The allowlist does not include `ioctl`, so the tracee is stopped with `code = 2` through no fault of its own. Pseudo-terminals, as opened by terminal emulators, are recognized without this call.

### Time measurement caveats

* The sandbox adds a limited amount of overhead measured in tracee time. On average, it adds ~20% to the time of each syscall. A typical algorithmic solution spends less than 5% of its time in kernel space, so its total overhead is ~1%.
* Time limit enforcement is precise: `time_limit <= time_usage <= time_limit + 10` for a tracee that spends its time computing.
  * A tracee that instead makes a syscall on nearly every scheduler slice can overshoot further, by up to 200 ms in our measurements. We observed this only in CI, never on a bare-metal machine, so it may be specific to virtualized environments.
* The CPU time measurement is stable provided that the tracee has undivided access to cache. Measurements can differ in multiprocess environments.

### Memory measurement caveats

* `memory_limit` (enforced via `RLIMIT_AS`) always caps what the tracee can actually allocate, independent of what gets reported.
* The reported `memory_usage` is the `VmPeak` value from `/proc/<pid>/status`, read right before the tracee exits or is killed by a signal, a timeout or an illegal syscall.

### Extra caveats

* In rare cases, the sandbox may stop a seemingly innocent tracee for calling the `SYS_futex` syscall. This happens when the tracee writes to memory within its address space that it did not allocate. Such a write raises no `SIGSEGV`, but it may corrupt internal glibc synchronization structures. Later, glibc may try to lock or unlock them by calling `SYS_futex`, which is blocked.
* A tracee that makes a 32-bit (i386 ABI) syscall, for example through a hand-written `int 0x80` in inline assembly, is killed immediately with `code = 1` and `subcode = SIGSYS`. It bypasses the usual illegal-syscall reporting (`code = 2`). No C/C++/Rust compiler targeting x86_64 emits such a syscall.
* The list of paths that the tracee may open (see `Config::get_openat_allowed_paths` in `config.h`) includes the time zone file `/usr/share/zoneinfo/Europe/Warsaw`. A tracee that loads any other time zone is stopped with `code = 2`. Adjust the list for your location.


## Testing

The test suite runs the sandbox through a separate executable, `sandbox_coverage`, built with coverage instrumentation. It is `EXCLUDE_FROM_ALL`, so plain `make` does not build it. Build it explicitly first:

```bash
$> cmake .
$> make sandbox_coverage
```

Tests are written with [pytest](https://pytest.org) and run through [uv](https://docs.astral.sh/uv/), which is not required for building or running the sandbox itself:

```bash
$> uv sync
$> uv run pytest -v tests
```

To run a single test:

```bash
$> uv run pytest -v tests/test_sandbox.py::test_prog_hello_world
```

See [testing.md](testing.md) for the full catalog of test programs and what each one exercises.
