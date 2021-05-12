#include <semaphore.h>
#include <iostream>
#include <pthread.h>

using namespace std;

#define NUM 100000                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     

sem_t mutex;
volatile int m = 0;

void *thread(void *vargp) {
    sem_wait(&mutex);
    cout << m << endl;
    ++m;
    cout << pthread_self() << " : done" << endl;
    sem_post(&mutex);
    return NULL;
}

int main() {
    sem_init(&mutex, 0, 1000);
    for (int i = 0; i < NUM; ++i) {
        pthread_t pid;
        pthread_create(&pid, NULL, thread, NULL);
        // pthread_join(pid, NULL);
    }    
    cout << "all done" << endl;
    pthread_exit(NULL);
    return 0;
}