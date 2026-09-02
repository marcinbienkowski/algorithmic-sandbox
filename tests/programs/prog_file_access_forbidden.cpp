#include <fstream>
#include <vector>

int main() {
    static const std::vector<std::string> paths = { "/proc/self/exe", "/proc/self/cgroup" };
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            file.close();
        }
    }
}
