#include <csignal>

int main() {
    raise(SIGABRT);
}
