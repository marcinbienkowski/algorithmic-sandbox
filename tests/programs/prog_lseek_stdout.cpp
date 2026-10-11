#include <unistd.h>

// Seeking back in stdout would let the tracee rewrite output it has already written.
int main() {
    lseek(STDOUT_FILENO, 0, SEEK_SET);
}
