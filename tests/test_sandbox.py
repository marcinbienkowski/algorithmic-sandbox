"""Test suite for sandbox system."""

import re
import resource
import shutil
import signal
import subprocess
import sys
import tempfile
from collections.abc import Callable
from pathlib import Path
from typing import IO

import pyseccomp
import pytest

from conftest import tests_options


def _syscall_nr(name: str) -> int:
    return pyseccomp.resolve_syscall(pyseccomp.Arch.X86_64, name)  # type: ignore[no-any-return]


EXIT_EXECVE_ERROR = (-1, 0)
EXIT_OK = (0, 0)
EXIT_TIMEOUT = (1, signal.SIGPROF)


def compile_program(source_file: Path, binary_file: Path) -> None:
    bs = ['-o', str(binary_file), str(source_file)]
    compilers = {
        '.c': ['/usr/bin/gcc', '-std=gnu18', '-Wall', '-Wextra', '-Wshadow', '-static', '-O2', '-s', *bs, '-lm'],
        '.cpp': ['/usr/bin/g++', '-std=gnu++20', '-Wall', '-Wextra', '-Wshadow', '-static', '-O2', '-s', *bs],
        '.rs': ['/usr/bin/rustc', '--edition=2021', '-C', 'opt-level=2', '-C', 'target-feature=+crt-static', '-C',
                'link-arg=-s', *bs],
    }
    result = subprocess.run(compilers[source_file.suffix], check=False, capture_output=True, text=True)  # noqa: S603
    assert result.stdout == ''
    assert result.stderr == ''
    result.check_returncode()


def compile_in_tempdir(temp_path: Path, source_file: Path) -> Path:
    temp_source = temp_path / source_file.name
    temp_source.write_text(source_file.read_text())
    binary = temp_path / 'code'
    compile_program(temp_source, binary)
    return binary


def run_sandbox_raw(executable: Path, result_file: Path, memory_limit: int = 16 * 1024, time_limit_in_ms: int = 500,
                    *, preexec_fn: Callable[[], None] | None = None, wrapper: list[str] | None = None,
                    stdin: IO[str] | None = None, print_debug: bool = False) -> subprocess.CompletedProcess[str]:
    argv = [*(wrapper or []), Path.cwd() / 'sandbox_coverage', str(memory_limit), str(time_limit_in_ms),
           executable, result_file, 'true' if print_debug else 'false']
    return subprocess.run(argv, check=False, stdin=stdin, capture_output=True, text=True,  # noqa: S603
                         preexec_fn=preexec_fn)


def assert_internal_error(sp: subprocess.CompletedProcess[str], result_file: Path, message: str | None = None) -> None:
    assert sp.returncode == 1
    if message is not None:
        result_text = result_file.read_text()
        assert 'Internal error' in result_text
        assert message in result_text


def compile_and_run(source_file: Path, input_file: Path, memory_limit: int, time_limit_in_ms: int,
                    executable: str) -> tuple[list[int], str, str]:
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        (temp_path / 'empty_file').touch()
        # we always compile to 'code', but we run `executable` later.
        compile_in_tempdir(temp_path, source_file)
        with input_file.open() as file:
            sp = run_sandbox_raw(temp_path / executable, temp_path / 'result', memory_limit, time_limit_in_ms,
                                stdin=file, print_debug=tests_options.debug_mode)
            sp.check_returncode()
        with (temp_path / 'result').open() as result:
            match = re.search(r'(-?\d+) (-?\d+) (\d+) (\d+)', result.read())
            assert match
            m = [int(x) for x in match.groups()]
            print(f'Sandbox result: {m}', file=sys.stderr)
        return m, sp.stdout, sp.stderr


def run_test(source_file: str, *, expected_exit_codes: list[tuple[int, int]] | None = None,
             memory_limit: int = 16 * 1024, time_limit: int = 500,
             min_time: int = 0, max_time: int = 100, min_memory: int = 768, max_memory: int = 3000,
             input_file: str = 'empty.txt', expected_output_file: str | None = None,
             executable: str = 'code', ignore_stderr: bool = False) -> None:
    if expected_exit_codes is None:
        expected_exit_codes = [EXIT_OK]
    path = Path.cwd() / 'tests'
    res, output_stdout, output_stderr = compile_and_run(path / 'programs' / source_file, path / 'in_out' / input_file,
                                                        memory_limit, time_limit, executable)
    if tests_options.debug_mode:
        print(output_stderr, file=sys.stderr)
    if not ignore_stderr and not tests_options.debug_mode:
        assert output_stderr == ''
    assert (res[0], res[1]) in expected_exit_codes
    assert min_memory <= res[3] <= max_memory
    assert min_time <= res[2] <= max_time
    if expected_output_file:
        assert output_stdout == (path / 'in_out' / expected_output_file).read_text()


""" Testing sandbox with wrong arguments """

def test_sandbox_no_arguments() -> None:
    sp = subprocess.run([Path.cwd() / 'sandbox_coverage'], check=False, capture_output=True, text=True)  # noqa: S603
    assert sp.returncode == 1
    assert 'mem_limit_in_kb time_limit_in_ms program_binary result_file print_debug(true/false)' in sp.stderr


def test_sandbox_nonexistent_file() -> None:
    run_test('prog_hello_world.cpp', executable='non_existent_file',
             expected_exit_codes=[EXIT_EXECVE_ERROR], min_memory=0)


def test_sandbox_nonexecutable_file() -> None:
    run_test('prog_hello_world.cpp', executable='empty_file',
             expected_exit_codes=[EXIT_EXECVE_ERROR], min_memory=0)


def test_sandbox_internal_error_on_setrlimit(tmp_path: Path) -> None:
    def lower_cpu_hard_limit() -> None:
        resource.setrlimit(resource.RLIMIT_CPU, (2, 2))
    binary = compile_in_tempdir(tmp_path, Path.cwd() / 'tests' / 'programs' / 'prog_hello_world.cpp')
    sp = run_sandbox_raw(binary, tmp_path / 'result', preexec_fn=lower_cpu_hard_limit)
    assert_internal_error(sp, tmp_path / 'result', 'setrlimit')


def test_sandbox_internal_error_on_ptrace_traceme(tmp_path: Path) -> None:
    strace = shutil.which('strace')
    assert strace is not None
    binary = compile_in_tempdir(tmp_path, Path.cwd() / 'tests' / 'programs' / 'prog_hello_world.cpp')
    sp = run_sandbox_raw(binary, tmp_path / 'result', wrapper=[strace, '-f', '-o', '/dev/null'])
    assert_internal_error(sp, tmp_path / 'result', 'PTRACE_TRACEME or BPF filter install')


def test_sandbox_internal_error_unwritable_result_file(tmp_path: Path) -> None:
    binary = compile_in_tempdir(tmp_path, Path.cwd() / 'tests' / 'programs' / 'prog_hello_world.cpp')
    result_file = tmp_path / 'no_such_dir' / 'result'
    sp = run_sandbox_raw(binary, result_file)
    assert_internal_error(sp, result_file)
    assert sp.stderr == ''
    assert not (tmp_path / 'no_such_dir').exists()


""" These programs should execute normally """

def test_prog_env_variables() -> None:
    run_test('prog_env_variables.cpp', expected_output_file='empty.txt')


def test_prog_file_access_allowed() -> None:
    run_test('prog_file_access_allowed.cpp')


def test_prog_hello_world() -> None:
    run_test('prog_hello_world.cpp')


def test_prog_many_writes() -> None:
    run_test('prog_many_writes.cpp', max_time=500)


def test_prog_memory_remap() -> None:
    run_test('prog_memory_remap.cpp', max_memory=6 * 1024)


@pytest.mark.parametrize('source_file',
                         ['prog_bubble_sort.cpp', 'prog_bubble_sort.c', 'prog_bubble_sort.rs'])
def test_prog_bubble_sort(source_file: str) -> None:
    run_test(source_file, input_file='numbers.in', expected_output_file='numbers_sorted.out')


@pytest.mark.parametrize('source_file',
                         ['prog_fibonacci.c', 'prog_fibonacci.cpp', 'prog_fibonacci.rs'])
def test_prog_fibonacci(source_file: str) -> None:
    run_test(source_file, expected_output_file='fibonacci.out')


@pytest.mark.parametrize('source_file',
                         ['prog_prime_numbers.c', 'prog_prime_numbers.cpp', 'prog_prime_numbers.rs'])
def test_prog_prime_numbers(source_file: str) -> None:
    run_test(source_file, input_file='numbers.in', expected_output_file='numbers_primes.out')


""" These programs should exit with some errors """

def test_prog_divide_by_zero() -> None:
    run_test('prog_divide_by_zero.cpp', expected_exit_codes=[(1, signal.SIGILL)])


def test_prog_divide_by_zero_asm() -> None:
    run_test('prog_divide_by_zero_asm.cpp', expected_exit_codes=[(1, signal.SIGFPE)])


def test_prog_infinite_loop() -> None:
    run_test('prog_infinite_loop.cpp', expected_exit_codes=[EXIT_TIMEOUT], min_time=500, max_time=510)


def test_prog_int3() -> None:
    run_test('prog_int3.cpp', expected_exit_codes=[(1, signal.SIGTRAP)])


def test_prog_large_local() -> None:
    run_test('prog_large_local.cpp', expected_exit_codes=[(1, signal.SIGSEGV)], max_memory=16 * 1024)


def test_prog_large_static() -> None:
    run_test('prog_large_static.cpp', expected_exit_codes=[(1, signal.SIGSEGV)])


def test_prog_malloc_bounded() -> None:
    run_test('prog_malloc_bounded.cpp', min_memory=16000, max_memory=16 * 1024)


def test_prog_malloc_unbounded() -> None:
    run_test('prog_malloc_unbounded.cpp', expected_exit_codes=[(1, signal.SIGSEGV)],
             min_memory=15500, max_memory=16 * 1024, max_time=530)


def test_prog_recursive_fibonacci() -> None:
    run_test('prog_recursive_fibonacci.cpp', expected_exit_codes=[EXIT_TIMEOUT], min_time=500, max_time=510)


# SIGABRT with gcc 13, EXIT_OK with gcc 14
def test_prog_ret2libc() -> None:
    run_test('prog_ret2libc.cpp', expected_exit_codes=[(1, signal.SIGABRT), EXIT_OK], ignore_stderr=True)


# Tracee spends almost all its time in the signal-delivery-stop.
# Looser timeout as the kill can only take effect once the tracer has been scheduled to run its signal handler.
def test_prog_storm_fault() -> None:
    run_test('prog_storm_fault.cpp', expected_exit_codes=[EXIT_TIMEOUT], min_time=490, max_time=700)


# mmap() counterpart of the test above.
def test_prog_storm_mmap() -> None:
    run_test('prog_storm_mmap.cpp', expected_exit_codes=[EXIT_TIMEOUT], min_time=490, max_time=700)


# Repeated at a short time limit to have a high chance of hitting the kill-during-an-already-reaped-seccomp-stop race.
# max_time is perhaps overgenerous here.
def test_prog_storm_mmap_repeated() -> None:
    for _ in range(10):
        run_test('prog_storm_mmap.cpp', expected_exit_codes=[EXIT_TIMEOUT], time_limit=100,
                 min_time=90, max_time=1000)


def test_prog_stack_overflow() -> None:
    run_test('prog_stack_overflow.cpp', expected_exit_codes=[(1, signal.SIGSEGV)],
             min_memory=16000, max_memory=16 * 1024)


def test_prog_stack_overflow_rust() -> None:
    run_test('prog_stack_overflow.rs', expected_exit_codes=[(1, signal.SIGSEGV)],
             min_memory=16000, max_memory=16 * 1024)


def test_prog_vector_growth() -> None:
    run_test('prog_vector_growth.cpp', expected_exit_codes=[(1, signal.SIGABRT)], ignore_stderr=True,
             min_memory=14000, max_memory=16 * 1024)


def test_prog_very_many_writes() -> None:
    run_test('prog_very_many_writes.cpp', expected_exit_codes=[EXIT_TIMEOUT], min_time=500, max_time=510)


""" These programs should be blocked as they use forbidden syscalls """

def test_prog_execve() -> None:
    run_test('prog_execve.cpp', expected_exit_codes=[(2, _syscall_nr('execve'))])


def test_prog_execve_asm() -> None:
    run_test('prog_execve_asm.cpp', expected_exit_codes=[(2, _syscall_nr('execve'))])


def test_prog_file_access_forbidden() -> None:
    run_test('prog_file_access_forbidden.cpp', expected_exit_codes=[(2, _syscall_nr('openat'))])


# SYS_sigprocmask on glibc >= 2.41, SYS_clone on older glibc.
def test_prog_fork() -> None:
    run_test('prog_fork.cpp', expected_exit_codes=[(2, _syscall_nr('clone')), (2, _syscall_nr('rt_sigprocmask'))])


def test_prog_getrlimit() -> None:
    run_test('prog_getrlimit.cpp', expected_exit_codes=[(2, _syscall_nr('prlimit64'))])


def test_prog_i386_abi() -> None:
    run_test('prog_i386_abi.cpp', expected_exit_codes=[(1, signal.SIGSYS)])


def test_prog_inotify() -> None:
    run_test('prog_inotify.cpp', expected_exit_codes=[(2, _syscall_nr('inotify_init1'))])


def test_prog_mmap_shared_stdout() -> None:
    run_test('prog_mmap_shared_stdout.cpp', expected_exit_codes=[(2, _syscall_nr('mmap'))])


def test_prog_open_wrong_address() -> None:
    run_test('prog_open_wrong_address.cpp', expected_exit_codes=[(2, _syscall_nr('openat'))])


def test_prog_open_wrong_flags() -> None:
    run_test('prog_open_wrong_flags.cpp', expected_exit_codes=[(2, _syscall_nr('openat'))])


def test_prog_readlinkat_wrong_path() -> None:
    run_test('prog_readlinkat_wrong_path.cpp', expected_exit_codes=[(2, _syscall_nr('readlinkat'))])


# Rejected on changing signal handlers before actually executing SYS_clone3.
def test_prog_pthread() -> None:
    run_test('prog_pthread.cpp', expected_exit_codes=[(2, _syscall_nr('rt_sigaction'))])


def test_prog_self_sigabrt() -> None:
    run_test('prog_self_sigabrt.cpp', expected_exit_codes=[(1, signal.SIGABRT)])


def test_prog_self_sigprof() -> None:
    run_test('prog_self_sigprof.cpp', expected_exit_codes=[(2, _syscall_nr('tgkill'))])


def test_prog_self_sigstop() -> None:
    run_test('prog_self_sigstop.cpp', expected_exit_codes=[(2, _syscall_nr('tgkill'))])


def test_prog_sleep() -> None:
    run_test('prog_sleep.cpp', expected_exit_codes=[(2, _syscall_nr('clock_nanosleep'))])


def test_prog_socket() -> None:
    run_test('prog_socket.cpp', expected_exit_codes=[(2, _syscall_nr('socket'))])


# Rejected on changing signal handlers before actually executing SYS_clone3.
def test_prog_system_call() -> None:
    run_test('prog_system_call.cpp', expected_exit_codes=[(2, _syscall_nr('rt_sigaction'))])
