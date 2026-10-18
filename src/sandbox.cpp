#include "subprocess.h"
#include "tracer.h"


int main(int argc, char* argv[]) {
    const std::vector args(argv, std::next(argv, argc));
    if (args.size() != 6) {
        std::cerr << args[0] << " mem_limit_in_kb time_limit_in_ms program_binary result_file print_debug(true/false)\n";
        return EXIT_FAILURE;
    }
    parameters params{args};
    const bool fds_closed = close_range(3, ~0U, 0) == 0;  // the first file tracee opens has fd=3
    const pid_t tracee_pid = fds_closed ? fork() : -1;
    if (tracee_pid == 0) {
        const Subprocess sp{std::move(params)};
        sp.set_limits();
        sp.execute_program();
    } else {  // LCOV_EXCL_LINE
        try {
            Util::ensure(fds_closed, "close_range");
            Util::ensure(tracee_pid >= 0);
            Tracer tracer{tracee_pid, params};
            if (tracer.initialize())
                while (tracer.process_event()) {}
            return EXIT_SUCCESS;
        } catch (const std::exception& e) {
            std::ofstream file{params.result_file, std::fstream::out};
            file << "Internal error: " << e.what();
            if (errno)
                file << ", errno = " << errno << ": " << std::system_category().message(errno);
            file << '\n';
            file.close();
            return EXIT_FAILURE;
        }
    }
}
