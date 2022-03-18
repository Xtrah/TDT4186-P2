#include "bbuffer.h"
#include "sem.h"
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct BNDBUF {
    SEM *full;
    SEM *empty;
    pthread_mutex_t lock;
	size_t head;
	size_t tail;
	size_t max; // Maximum amount of values that can be stored in the buffer
	int count; // Amount of values in the buffer
    int *buffer;
} BNDBUF;


BNDBUF *bb_init(unsigned int size) {
    assert(size > 0);
    BNDBUF *buf = (BNDBUF*)malloc(sizeof(BNDBUF));
    buf->buffer = malloc(sizeof(int) * size);
    buf->max = size;
    buf->head = 0;
    buf->tail = 0;
    // buf->count = 0;
    pthread_mutex_init(&buf->lock, NULL);
    buf->full = sem_init(size);
    buf->empty = sem_init(0);

    return buf;
}

void bb_del(BNDBUF *bb) {
    free(bb->buffer);
    sem_del(bb->full);
    sem_del(bb->empty);
    free(bb);

}

int bb_get(BNDBUF *bb) {
    // while (bb->count <= 0) {
    //     P(bb->empty); // Wait for count to be positive.
    // }
    P(bb->empty);

    pthread_mutex_lock(&bb->lock);
    int value = bb->buffer[bb->head]; // Value of head
    bb->head = (bb->head + 1) % bb->max; // Move buffer head one step further (wrap around max).
    bb->count--; // Decrement count (because there is one less value in the buffer)
    pthread_mutex_unlock(&bb->lock);
    
    V(bb->full);
    return value;
}

void bb_add(BNDBUF *bb, int fd) {
    // while (bb->count >= bb->max) {
    //     P(bb->full);
    // }
    P(bb->full);

    pthread_mutex_lock(&bb->lock);
    bb->buffer[bb->tail] = fd; // Insert value fd into buffer tail slot.
    bb->tail = (bb->tail + 1) % bb->max; // Move buffer tail one step further (wrap around max).
    bb->count++; // Increment number of elements in buffer.
    pthread_mutex_unlock(&bb->lock);
    
    V(bb->empty); // Signal that count has been increased.
}
