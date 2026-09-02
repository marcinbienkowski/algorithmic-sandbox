#include <fcntl.h>
#include <unistd.h>

int main() {
    char buf[64];
    ssize_t ret = readlinkat(AT_FDCWD, "/etc/hostname", buf, sizeof(buf));  // not in the readlinkat allowlist
    return ret != -1;
}
