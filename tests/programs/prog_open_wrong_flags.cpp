#include <fcntl.h>

int main() {
    open("/proc/self/maps", O_RDWR);
}
