#include "bbuffer.h"
#include "sem.h"

#include <pthread.h>
#include <stdio.h>
#include <assert.h>

BNDBUF *buffer;

SEM *sem1;
SEM *sem2;
SEM *sem3;

void *fun1() {
    P(sem2);
    int val = bb_get(buffer);
    printf("Get: %d\n", val);
    V(sem3);

    return NULL;
}

void *fun2() {
    P(sem1);
    bb_add(buffer, 69);
    V(sem2);

    return NULL;
}

int main() {
    buffer = bb_init(3);

    sem1 = sem_init(1);
    sem2 = sem_init(0);
    sem3 = sem_init(0);

    pthread_t pt1;
    int r1 = pthread_create(&pt1, NULL, fun1, NULL);
    assert(r1 == 0);
    
    pthread_t pt2;
    int r2 = pthread_create(&pt2, NULL, fun2, NULL);
    assert(r2 == 0);

    P(sem3);

    return 0;
}