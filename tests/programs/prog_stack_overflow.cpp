#include <iostream>

void recursive(int depth) {
    [[maybe_unused]] int x = 42;
    if (depth > 0)
        recursive(depth - 1);
    std::cout << x << std::endl;
}

int main() { 
    recursive(10000000);
}
