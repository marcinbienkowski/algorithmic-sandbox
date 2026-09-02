int main() {
    asm volatile(
        "movl $1, %eax;"
        "movl $0, %ecx;"
        "divl %ecx"
    );
}
