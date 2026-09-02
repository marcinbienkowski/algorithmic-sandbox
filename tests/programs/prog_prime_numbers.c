#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

enum { MAX_NUM = 1000000 };

bool* sieve() {
    bool* is_prime = malloc((MAX_NUM + 1) * sizeof(bool));
    for (int i = 0; i <= MAX_NUM; ++i)
        is_prime[i] = true;
    is_prime[0] = is_prime[1] = false;
    for (long long i = 2; i * i <= MAX_NUM; ++i)
        if (is_prime[i])
            for (long long j = i * i; j <= MAX_NUM; j += i)
                is_prime[j] = false;
    return is_prime;
}

int main() {
    const bool* is_prime = sieve();
    int n;
    assert(scanf("%d", &n) == 1);
    for (int i = 0; i < n; ++i) {
        int num;
        assert(scanf("%d", &num) == 1);
        printf("%s\n", (num <= MAX_NUM && is_prime[num]) ? "YES" : "NO");
    }
    return 0;
}
