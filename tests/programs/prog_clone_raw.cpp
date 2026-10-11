#include <csignal>
#include <sys/syscall.h>
#include <unistd.h>

// Raw clone(), as glibc's fork(), pthread_create() and system() are rejected earlier on glibc >= 2.41.
int main() {
    syscall(SYS_clone, SIGCHLD, 0, 0, 0, 0);
}
