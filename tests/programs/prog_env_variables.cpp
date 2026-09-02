#include <iostream>

int main(int, char**, char *envp[]) {
    for (char **env = envp; *env != nullptr; env++) {
        std::cout << *env << std::endl;
    }
}
