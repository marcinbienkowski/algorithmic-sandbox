#include <fstream>
#include <vector>

int main() {
    static const std::vector<std::string> paths = { "/etc/timezone", "/dev/urandom", "/proc/self/maps", "/usr/share/zoneinfo/Europe/Warsaw" };
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            file.close();
        }
    }
}
