#include "sem.h"
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

typedef struct SEM {
    int value;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} SEM;


SEM *sem_init(int initVal) {
    SEM *sem = (SEM*)malloc(sizeof(SEM));
    sem->value = initVal;
    if (pthread_mutex_init(&sem->lock, NULL) < 0) return NULL;
    if (pthread_cond_init(&sem->cond, NULL) < 0) return NULL;

    return sem;
}

int sem_del(SEM *sem) {
    free(sem);
    return 0;
}

void P(SEM *sem) {
    assert(pthread_mutex_lock(&sem->lock) == 0);
    sem->value--; // Decrement
    while (sem->value < 0) {
        pthread_cond_wait(&sem->cond, &sem->lock); //  blocks the calling thread, waiting for the condition specified by cond to be signaled or broadcast to
    }
    pthread_cond_signal(&sem->cond); // unblock at least one of the threads that are blocked on the specified condition variable cond
    assert(pthread_mutex_unlock(&sem->lock) == 0);
}

void V(SEM *sem) {
    assert(pthread_mutex_lock(&sem->lock) == 0);
    sem->value++; // Increment
    pthread_cond_signal(&sem->cond);
    assert(pthread_mutex_unlock(&sem->lock) == 0);
}

