#include <iostream>

int main() {
    constexpr int n = 10'000'000;
    int tab[n];  // 40 MB
    for (int i = 0; i < n; i++)
        tab[i] = i;
    std::cout << tab[42] << std::endl;
}
