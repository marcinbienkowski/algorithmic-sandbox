# Algorithmic Sandbox

[![Local Tests](https://github.com/marcinbienkowski/algorithmic-sandbox/actions/workflows/tests.yml/badge.svg?branch=main)](https://github.com/marcinbienkowski/algorithmic-sandbox/actions/workflows/tests.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.txt)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](CMakeLists.txt)
[![Platform](https://img.shields.io/badge/platform-Linux%20x86__64-lightgrey.svg)](#extra-caveats)

A secure sandbox for running submissions to algorithmic-problem competitions (Codeforces-style, as on most online judges): untrusted binary code (referred to as "tracee" below) that reads a test case from `stdin` and writes its answer to `stdout`, executed under strict security constraints. It prevents malicious or resource-intensive tracees from affecting the host system.
It has been **battle-tested on over 100,000 real algorithmic solutions** and has proven rock-stable in production.

It enforces memory and time limits. It uses a seccomp-BPF filter with `ptrace()` to monitor and restrict system calls, allowing only a safe subset. It allows tracees that perform computations and use `stdin`/`stdout`/`stderr`, and blocks everything else. In particular, it forbids operations like file access, network communication, and process creation.

The goal is to block malicious tracees while allowing legitimate tracees to run. The sandbox is designed to err on the side of caution:
* A malicious tracee will *always* be blocked.
* A legitimate tracee will *usually* run uninterrupted. This holds for computational tracees written in C/C++/Rust (see example programs in `tests/programs`) that have been statically compiled. (Static compilation avoids the need for opening shared libraries at runtime, which would be blocked.)



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

For a tracee that reads a test case from `stdin` and writes its answer to `stdout` (as in a typical online-judge problem), run it against a test case by redirecting the sandbox's own `stdin`/`stdout`, which the tracee inherits (see [Input and output](#input-and-output) below):

```bash
$> g++ -O2 -static -o prog_bubble_sort tests/programs/prog_bubble_sort.cpp
$> ./sandbox 4096 1000 ./prog_bubble_sort result-file.txt false < tests/in_out/numbers.in > answer.out
$> cat answer.out
```

### Returned values

If sandboxing was successful, the sandbox returns `0` and writes four numbers to
`<result_file>`:
* `code`: 
  * `0` if the tracee exited normally;
  * `1` if the tracee was killed by a signal;
  * `2` if the tracee was aborted by the sandbox due to an illegal system call;
  * `-1` if the sandbox failed to start the tracee (for example, when the binary does not exist or is not executable).
* `subcode`: exit status if `code = 0`, delivered signal if `code = 1` and syscall number if `code = 2`.
* `time_usage` of the tracee in milliseconds.
* `memory_usage` of the tracee in kilobytes equal to the peak virtual memory
  allocation.

If an internal error occurs, the sandbox returns `1` and the `<result_file>` contains a description of the error.  Even in such cases, the tracee will be executed safely, although it may terminate prematurely.  

The value of `memory_usage` will never exceed `memory_limit`.

If the tracee's actual runtime (user time + system time) exceeds `time_limit`, the sandbox will terminate the tracee and return `code = 1` with `subcode = 27 (SIGPROF)`. The timeout is enforced by a watchdog running in the *sandbox process*, armed on the tracee's own CPU-time clock. The tracee cannot evade the timeout. 

If `print_debug` is equal to the string `true`, then additional information about executed syscalls is output to `stderr`. 

### Input and output

The sandboxed tracee inherits `stdin`, `stdout`, and `stderr` of the sandbox process. You can redirect them as needed. The sandbox itself never reads anything from `stdin`, never writes anything to `stdout`, and only writes to `stderr` if `print_debug` is set to `true`.

Redirect `stdin` from a regular file, and have `stdout`/`stderr` go to a regular file or a pipe that is actively drained — do not leave them attached to a terminal. There is no wall-clock limit anywhere in the sandbox, only CPU-time ones, so a blocking `read` on an empty, non-redirected `stdin` or a `write` to a full, undrained pipe hangs the tracee forever without tripping the timeout. A terminal carries a second risk on top of that: libc's startup code probes a tty via `ioctl`, a syscall that the sandbox allowlist does not include, so the tracee is killed as if it had made an illegal call (`code = 2`) through no fault of the tracee itself.

### Time measurement caveats

* The sandbox adds a limited amount of overhead measured in tracee time. On average, there is ~20% overhead for each syscall, which, for a typical algorithmic solution that spends <5% of its time in kernel space, translates to ~1% overhead. 
* Time limit enforcement is precise: `time_limit <= time_usage <= time_limit + 10` for a tracee that spends its time computing. 
  * A tracee that instead spends its time making a syscall on nearly every scheduler slice can overshoot further: measured up to `time_limit + 200`.
  * This overshoot is CI-specific: it was never reproduced on a bare-metal dev machine, so perhaps it occurs only in a virtualized environment.
* Note that the CPU time measurement is stable provided that the tracee has undivided access to cache, and the measurements can differ in multiprocess environments.

### Memory measurement caveats

* `memory_limit` (enforced via `RLIMIT_AS`) always caps what the tracee can actually allocate, independent of what gets reported.
* The reported `memory_usage` is the peak `VmPeak` seen in `/proc/<pid>/status`, sampled around every syscall that can grow it (`mmap`/`brk`/`mremap`) and once more right before the tracee exits. 
  * That last read is crucial, as stack growth can raise `VmPeak` through page faults, without triggering a syscall. 
  * If the watchdog's `SIGKILL` lands while the tracee sits in an already-reaped ptrace-stop, that final pre-exit read is skipped entirely. We still try to read `VmPeak` then, but it is theoretically possible that it fails, in which case the last syscall-adjacent sample is reported instead.
  * That final read has never been observed to fail in practice.


### Extra caveats

* In rare cases, the sandbox may stop a seemingly innocent tracee because it uses the `SYS_futex` syscall. This happens when the tracee writes to memory that was not allocated by the tracee, but is still within the tracee's address space. In such a case the `SIGSEGV` signal is not generated, but such a write may corrupt internal glibc synchronization structures. This may eventually lead to glibc trying to perform a lock/unlock operation, calling `SYS_futex`, which is blocked.
* A tracee that makes a 32-bit (i386 ABI) syscall — for example a hand-written `int 0x80` in inline assembly, never emitted by a C/C++/Rust compiler targeting x86_64 — is killed immediately with `code = 1` and `subcode = SIGSYS`, rather than going through the usual illegal-syscall reporting (`code = 2`).

## Testing

The test suite runs the sandbox via a separate `sandbox_coverage` executable, not `sandbox` itself. It's `EXCLUDE_FROM_ALL`, so plain `make` does not build it — build it explicitly first:

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
