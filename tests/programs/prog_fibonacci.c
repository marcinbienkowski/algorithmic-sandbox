#include <stdio.h>

long long fibonacci(int n) {
    if (n <= 1)
		return n;
    long long a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        const long long c = a + b;
        a = b;
        b = c;
    }
    return b;
}

int main() {
    int n = 20;
    for (int i = 0; i <= n; i++)
        printf("%lld\n", fibonacci(i));
    return 0;
}
