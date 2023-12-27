#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include "rwlock.h"

// ta mitchell helped with struct
typedef struct rwlock {
    int p;
    int n;
    pthread_mutex_t lock;
    pthread_cond_t reader, writer;
    int active_readers;
    int active_writers;
    int waiting_readers;
    int waiting_writers;
    int nwaycount;
    bool flag;
} rwlock_t;

rwlock_t *rwlock_new(PRIORITY p, uint32_t n) {
    // n readers between each writer
    rwlock_t *new_rwlock = (rwlock_t *) malloc(sizeof(rwlock_t));

    if (new_rwlock == NULL) {
        return NULL;
    }

    // initializing all attributes
    new_rwlock->p = p;
    new_rwlock->n = n;

    new_rwlock->active_readers = 0;
    new_rwlock->active_writers = 0;

    new_rwlock->waiting_readers = 0;
    new_rwlock->waiting_writers = 0;

    new_rwlock->nwaycount = 0;
    new_rwlock->flag = false;

    pthread_mutex_init(&new_rwlock->lock, NULL);
    pthread_cond_init(&new_rwlock->reader, NULL);
    pthread_cond_init(&new_rwlock->writer, NULL);

    return new_rwlock;
}

/** @brief Delete your rwlock and free all of its memory.
 *
 *  @param rw the rwlock to be deleted.  Note, you should assign the
 *  passed in pointer to NULL when returning (i.e., you should set *rw
 *  = NULL after deallocation).
 *
 */
void rwlock_delete(rwlock_t **rw) {
    if (rw == NULL || *rw == NULL) {
        return;
    }

    rwlock_t *w = *rw;

    // destroying attributes
    pthread_mutex_destroy(&w->lock);
    pthread_cond_destroy(&w->reader);
    pthread_cond_destroy(&w->writer);

    free(w);
    *rw = NULL;
}

// ta mitchell helped with the following code with the pseudo he gave in section

/** @brief acquire rw for reading
 *
 */
void reader_lock(rwlock_t *rw) {

    // locking mutext
    int hold = 0;

    hold = pthread_mutex_lock(&rw->lock);

    if (hold != 0) {
        return;
    }

    // increment # of waiting readers
    rw->waiting_readers++;

    // make thread wait
    while (reader_wait(rw)) {
        pthread_cond_wait(&rw->reader, &rw->lock);
    }

    // decrement waiting readers and incrememnt active
    rw->waiting_readers--;
    rw->active_readers++;

    // if priority is nway
    if (rw->p == N_WAY) {
        if (rw->flag == true && rw->nwaycount == 0) {
            rw->flag = false;
        }
        if (rw->flag == false && rw->nwaycount < rw->n) {
            rw->nwaycount++;
        }
    }

    // unlock the mutex
    pthread_mutex_unlock(&rw->lock);
}

/** @brief release rw for reading--you can assume that the thread
 * releasing the lock has *already* acquired it for reading.
 *
 */
void reader_unlock(rwlock_t *rw) {

    // locking mutext
    int hold = 0;

    hold = pthread_mutex_lock(&rw->lock);

    if (hold != 0) {
        return;
    }

    // decrement active reader count
    rw->active_readers--;

    // if priority is nway
    if (rw->p == N_WAY) {
        if (rw->nwaycount == 0) {
            rw->flag = true;
        }
        if (rw->flag == true && rw->nwaycount) {
            rw->nwaycount = 0;
        }
    }

    // awaken all threads and unlock mutex
    pthread_cond_broadcast(&rw->reader);
    pthread_cond_broadcast(&rw->writer);
    pthread_mutex_unlock(&rw->lock);
}

/** @brief acquire rw for writing
 *
 */
void writer_lock(rwlock_t *rw) {

    // lock mutex
    int hold = 0;

    hold = pthread_mutex_lock(&rw->lock);

    if (hold != 0) {
        return;
    }

    // make thread wait
    while (writer_wait(rw)) {
        pthread_cond_wait(&rw->writer, &rw->lock);
    }

    // decrement waiting writer count and increment active writers
    rw->waiting_writers--;
    rw->active_writers++;

    // unlock mutex
    pthread_mutex_unlock(&rw->lock);
}

/** @brief release rw for writing--you can assume that the thread
 * releasing the lock has *already* acquired it for writing.
 *
 */
void writer_unlock(rwlock_t *rw) {

    // unlock mutex
    int hold = 0;

    hold = pthread_mutex_lock(&rw->lock);

    if (hold != 0) {
        return;
    }

    // increment active readers
    rw->active_readers++;

    // if priority is nway
    if (rw->p == N_WAY) {
        if (rw->nwaycount == 0) {
            rw->flag = true;
        }
        if (rw->flag == true && rw->nwaycount > 0) {
            rw->nwaycount = 0;
        }
    }

    // awaken threads and unlock mutex
    pthread_cond_broadcast(&rw->reader);
    pthread_cond_broadcast(&rw->writer);
    pthread_mutex_unlock(&rw->lock);
}

int reader_wait(rwlock_t *rw) {

    // if priority is readers
    if (rw->p == READERS) {
        return rw->active_writers;
    }
    // if priority is writers
    else if (rw->p == WRITERS) {
        return rw->waiting_writers;
    }
    // if priority is nway
    else if (rw->p == N_WAY) {
        // active readers is greater than 0
        if (rw->active_readers) {
            if (rw->waiting_writers) {
                if (rw->nwaycount < rw->n) {
                    return 0;
                } else {
                    if (rw->flag) {
                        return rw->waiting_writers;
                    }
                }
            } else {
                return 0;
            }
        }
        // active writers greater than z0
        else if (rw->active_writers) {
            return rw->active_writers;
        } else {
            if (rw->flag) {
                return rw->waiting_writers;
            } else {
                return 0;
            }
        }
    }
    // throw error
    else {
        fprintf(stderr, "Reader wait failed.\n");
        return -1;
    }

    return -1;
}

int writer_wait(rwlock_t *rw) {
    // if priority is readers
    if (rw->p == READERS) {
        return rw->active_readers;
    }
    // if priority is writers
    else if (rw->p == WRITERS) {
        return rw->active_writers;
    }
    // if priority is nway
    else if (rw->p == N_WAY) {
        // if active readers is greater than 0
        if (rw->active_readers) {
            return rw->active_readers;
        }
        // if active writers is greater than 0
        else if (rw->active_writers) {
            return rw->active_writers;
        } else {
            if (rw->flag == false) {
                return rw->waiting_readers;
            } else {
                return 0;
            }
        }
    }
    // throw error
    else {
        fprintf(stderr, "Writer wait failed.\n");
        return -1;
    }
}
