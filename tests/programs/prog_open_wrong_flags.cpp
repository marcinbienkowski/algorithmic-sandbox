#include <fcntl.h>

int main() {
    open("/dev/urandom", O_RDWR);
}
