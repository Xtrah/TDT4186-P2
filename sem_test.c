#include "sem.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

SEM *s1;
SEM *s2;
SEM *s3;

void *t1() {
    P(s1);
    printf("w");
    V(s2);
    P(s1);
    printf("d");
    V(s2);
    
    return NULL;
}

void *t2() {
    P(s2);
    printf("o");
    printf("r");
    V(s1);
    P(s2);
    printf("l");
    printf("e");
    V(s3);
    
    return NULL;
}

int main () {
    s1 = sem_init(1);
    s2 = sem_init(0);
    s3 = sem_init(0);
    
    pthread_t pt1;
    int r1 = pthread_create(&pt1, NULL, t1, NULL);
    assert(r1 == 0);
    
    pthread_t pt2;
    int r2 = pthread_create(&pt2, NULL, t2, NULL);
    assert(r2 == 0);
    
    P(s3);
    printf("\n");
    return 0;
}