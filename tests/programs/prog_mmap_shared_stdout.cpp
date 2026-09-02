#include <sys/mman.h>
#include <unistd.h>

int main() {
    mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, STDOUT_FILENO, 0);
}
