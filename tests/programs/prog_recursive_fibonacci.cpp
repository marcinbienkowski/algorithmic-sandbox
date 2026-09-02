#include <iostream>

long long fibonacci(int n) {
    if (n == 0 or n == 1)
        return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    constexpr int n = 50;
    for (int i = 0; i <= n; i++)
        std::cout << fibonacci(i) << std::endl;
}
