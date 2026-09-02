#include <sys/mman.h>

// mmap() is decided in user space, so this spends almost all of its time in a ptrace stop, giving a
// decent chance that the watchdog's SIGKILL lands on a stopped tracee. Remaps the same page with
// MAP_FIXED each time, so the mapping never grows.

int main() {
    void* page = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    for (;;)
        mmap(page, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
}
