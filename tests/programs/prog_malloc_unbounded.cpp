#include <cstring>
#include <iostream>

int main() {
    for (;;) {
        std::cout << "x" << std::endl;
        void *ptr = malloc(1024);
        memset(ptr, 'A', 1024);
    }
}
