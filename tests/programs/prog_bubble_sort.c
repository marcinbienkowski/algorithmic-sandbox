#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void bubble_sort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

int main() {
    int n;
    assert(scanf("%d", &n) == 1);
    int *arr = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
        assert(scanf("%d", &arr[i]) == 1);
    bubble_sort(arr, n);
    for (int i = 0; i < n; i++) {
        printf("%d\n", arr[i]);
    }
    return 0;
}
