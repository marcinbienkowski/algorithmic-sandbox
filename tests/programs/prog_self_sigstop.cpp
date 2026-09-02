#include <csignal>

int main() {
    raise(SIGSTOP);
}
