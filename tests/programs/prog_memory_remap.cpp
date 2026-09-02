#include <cstdlib>

int main() {
    char *ptr = static_cast<char*>(malloc(1 << 20));
    ptr = static_cast<char *>(realloc(ptr, 4 << 20));
    ptr[0] = 'A';
}
