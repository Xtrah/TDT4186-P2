#include "bbuffer.h"
#include "sem.h"
#include <assert.h>
#include <stdlib.h>

typedef struct BNDBUF {
    SEM *full;
    SEM *empty;
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
    buf->count = 0;
    
    buf->full = sem_init(0);
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
    while (bb->count <= 0) {
        P(bb->empty); // Wait for count to be positive.
    }

    int value = bb->buffer[bb->head]; // Value of head
    bb->head = (bb->head + 1) % bb->max; // Move buffer head one step further (wrap around max).
    bb->count--; // Decrement count (because there is one less value in the buffer)
    V(bb->full);
    return value;
}

void bb_add(BNDBUF *bb, int fd) {
    while (bb->count >= bb->max) {
        P(bb->full);
    }

    bb->buffer[bb->tail] = fd; // Insert value fd into buffer tail slot.
    bb->tail = (bb->tail + 1) % bb->max; // Move buffer tail one step further (wrap around max).
    bb->count++; // Increment number of elements in buffer.
    
    V(bb->empty); // Signal that count has been increased.
}
