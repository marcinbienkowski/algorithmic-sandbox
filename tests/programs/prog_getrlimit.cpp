#include <sys/resource.h>
#include <iostream>

void print_resource_limit(int resource, const std::string& name) {
    rlimit limit;
    getrlimit(resource, &limit);
    std::cout << name << ": " << static_cast<long long>(limit.rlim_cur) << ", " << static_cast<long long>(limit.rlim_max) << std::endl;
}

int main() {
    print_resource_limit(RLIMIT_STACK, "RLIMIT_STACK");
    print_resource_limit(RLIMIT_NOFILE, "RLIMIT_NOFILE");
    print_resource_limit(RLIMIT_CPU, "RLIMIT_CPU");
    print_resource_limit(RLIMIT_AS, "RLIMIT_AS");
    print_resource_limit(RLIMIT_DATA, "RLIMIT_DATA");
    print_resource_limit(RLIMIT_FSIZE, "RLIMIT_FSIZE");
    print_resource_limit(RLIMIT_CORE, "RLIMIT_CORE");
    print_resource_limit(RLIMIT_NPROC, "RLIMIT_NPROC");
    print_resource_limit(RLIMIT_LOCKS, "RLIMIT_LOCKS");
    print_resource_limit(RLIMIT_MEMLOCK, "RLIMIT_MEMLOCK");
    print_resource_limit(RLIMIT_MSGQUEUE, "RLIMIT_MSGQUEUE");
    print_resource_limit(RLIMIT_NICE, "RLIMIT_NICE");
    print_resource_limit(RLIMIT_RSS, "RLIMIT_RSS");
    print_resource_limit(RLIMIT_RTPRIO, "RLIMIT_RTPRIO");
    print_resource_limit(RLIMIT_RTTIME, "RLIMIT_RTTIME");
    print_resource_limit(RLIMIT_SIGPENDING, "RLIMIT_SIGPENDING");
}
