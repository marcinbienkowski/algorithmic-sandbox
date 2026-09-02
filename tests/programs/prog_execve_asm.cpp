int main() {
    asm volatile(
        "jmp code;"
        "path: .ascii \"/bin/ls\\0\";"
        "code: movq $59, %rax;"
        "leaq path(%rip), %rdi;"
        "movq $0, %rsi;"
        "movq $0, %rdx;"
        "syscall"
    );
}
