#include <iostream>
#include <vector>

constexpr int MAX_NUM = 1000000;

auto sieve() {
    std::vector<bool> is_prime(MAX_NUM + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (long long i = 2; i * i <= MAX_NUM; i++)
        if (is_prime[i])
            for (auto j = i * i; j <= MAX_NUM; j += i)
                is_prime[j] = false;
    return is_prime;
}

int main() {
    const auto is_prime = sieve();
    int n;
    std::cin >> n;
    for (int i = 0; i < n; ++i) {
        int num;
        std::cin >> num;
        std::cout << (num <= MAX_NUM and is_prime[num] ? "YES" : "NO") << '\n';
    }
}
