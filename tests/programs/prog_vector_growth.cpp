#include <iostream>
#include <vector>

int main() {
    std::vector<int> data;
    for (int i = 0; i < 100000000; i++) {
        data.push_back(i);
    }
    std::cout << data[0] << std::endl;
}
