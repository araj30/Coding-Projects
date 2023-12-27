#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>

#include "queue.h"

typedef struct queue {
    int N;
    int in;
    int out;
    void **buffer;
    sem_t empty;
    sem_t full;
    sem_t mutex;
} queue_t;

queue_t *queue_new(int size) {
    queue_t *new_q = (queue_t *) malloc(sizeof(queue_t));

    // initialize all attributes
    new_q->N = size;
    new_q->in = 0;
    new_q->out = 0;

    sem_init(&new_q->empty, 0, 1);
    sem_init(&new_q->full, 0, size);
    sem_init(&new_q->mutex, 0, 1);

    new_q->buffer = malloc(sizeof(void **) * size);

    if (new_q->buffer == NULL) {
        free(new_q);
        return NULL;
    }

    return new_q;
}

void queue_delete(queue_t **q) {
    // checkk that pointer to pointer- check if there's a valid queue structure
    // check that queue pointer stored in *q- check if queue itself exists
    // or if the pointer to the
    if (q == NULL || *q == NULL) {
        return;
    }

    if ((*q)->buffer != NULL) {
        free((*q)->buffer);
    }

    // destroy attributes
    sem_destroy(&(*q)->empty);
    sem_destroy(&(*q)->full);
    sem_destroy(&(*q)->mutex);

    (*q)->out = 0;
    (*q)->in = 0;

    free(*q);

    *q = NULL;
}

bool queue_push(queue_t *q, void *elem) {
    if (q == NULL || elem == NULL) {
        return false;
    }

    // check if space, wait for unlock
    sem_wait(&(q->full));
    sem_wait(&q->mutex);

    // pushing to correct index
    q->buffer[q->in] = elem;
    q->in = (q->in + 1) % q->N;

    // close out and let program know empty has a spot
    sem_post(&q->mutex);
    sem_post(&q->empty);

    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    if (q == NULL || elem == NULL) {
        return false;
    }

    // check if space, wait for unlock
    sem_wait(&(q->empty));
    sem_wait(&q->mutex);

    // popping from correct buffer
    *elem = q->buffer[q->out];
    q->out = (q->out + 1) % q->N;

    // close out and let program know full has spot
    sem_post(&q->mutex);
    sem_post(&q->full);

    return true;
}
