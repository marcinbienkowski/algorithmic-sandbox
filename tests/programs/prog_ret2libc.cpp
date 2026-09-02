#include <iostream>
#include <cstring>

// For this to work, one has to disable stack canaries by compiling with -fno-stack-protector -O0

void dangerous_function() {
    std::cout << "Dangerous" << std::endl;
}

void vulnerable_function(char* input) {
    char buffer[64];
    strcpy(buffer, input);
}

int main() {
    std::cout << "Target address: " << (void*)dangerous_function << std::endl;
    char payload[80];                 // 64 buffer + padding + return address
    memset(payload, 'A', 72);         // Fill buffer + saved RBP
    *(void**)(payload + 72) = (void*)dangerous_function;
    vulnerable_function(payload);
}
