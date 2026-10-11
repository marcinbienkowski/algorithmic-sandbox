#include <csignal>

static void handler(int) {}

int main() {
    std::signal(SIGFPE, handler);
}
