#include <csignal>

// Spends almost all of its time in a signal-delivery-stop, so there is a decent chance that
// the watchdog's SIGKILL lands on a stopped tracee.

static void handler(int) {}

int main() {
    struct sigaction sa {};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
    int* p = nullptr;
    *p = 42;
}
