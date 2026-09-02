int main() {
    // close(2) using i386 ABI
    asm volatile(
        "movl $6, %eax;"
        "movl $2, %ebx;"
        "int $0x80"
    );
}
