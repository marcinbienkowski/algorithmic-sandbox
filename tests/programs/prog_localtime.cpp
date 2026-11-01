#include <ctime>
#include <iostream>

int main() {
    const std::time_t now = std::time(nullptr);
    std::cout << std::localtime(&now)->tm_hour << "\n";
}
