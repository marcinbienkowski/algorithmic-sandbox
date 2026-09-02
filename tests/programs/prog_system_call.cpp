#include <iostream>
#include <cassert>

int main() {
    assert(system("/usr/bin/true") != 0);
}
