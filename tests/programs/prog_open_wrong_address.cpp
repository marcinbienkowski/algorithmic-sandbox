#include <fcntl.h>

int main() {
    open(reinterpret_cast<const char*>(42), O_RDONLY);  // wrong memory address
}
