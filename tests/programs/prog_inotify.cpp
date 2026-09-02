#include <sys/inotify.h>

int main() {
    inotify_init1(IN_CLOEXEC);
}
