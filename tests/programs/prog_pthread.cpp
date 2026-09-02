#include <pthread.h>
#include <iostream>

void* thread_function(void*) { 
    std::cout << "Hello world" << std::endl;
    return nullptr; 
}

int main() {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 64 * 1024);
    pthread_create(&thread, &attr, thread_function, nullptr);
}
