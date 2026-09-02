#include <unistd.h>

int main() {
    char* ptr[] = {nullptr};
    execve("/bin/ls", ptr, ptr);
}
