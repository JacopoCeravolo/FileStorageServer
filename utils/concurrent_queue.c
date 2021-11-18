#include <errno.h>
#include <stdlib.h>

#include "concurrent_queue.h"

concurrent_queue_t*
concurrent_queue_create(bool cmp(void*, void*), void free_fun(void*), void print(void*, FILE*))
{
    concurrent_queue_t *queue = calloc(1, sizeof(concurrent_queue_t));
    if (queue == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    queue->elems = list_create(cmp, free_fun, print);
    if (queue->elems == NULL) {
        return NULL;
    }

    if ((pthread_mutex_init(&(queue->q_lock), NULL)) != 0 ||
        (pthread_cond_init(&(queue->q_cond), NULL) != 0)) {
        free(queue->elems);
        free(queue);
        return NULL;
    }

    queue->q_size = 0;

    return queue;
}

int
concurrent_queue_destroy(concurrent_queue_t *queue)
{
    printf("still waiting: %d\n", queue->q_size);
    list_destroy(queue->elems);

    pthread_mutex_destroy(&(queue->q_lock));
    // pthread_cond_destroy(&(queue->q_cond));

    free(queue);

    return 0;
}

int
concurrent_queue_put(concurrent_queue_t *queue, void* data)
{
    if (pthread_mutex_lock(&(queue->q_lock)) != 0) { 
        return -1;
    }

    if (list_insert_tail(queue->elems, data) != 0) {
        return -1;
    }

    queue->q_size++;

    pthread_cond_signal(&(queue->q_cond));

    if (pthread_mutex_unlock(&(queue->q_lock)) != 0) { 
        return -1;
    }
    
    return 0;
}

void*
concurrent_queue_get(concurrent_queue_t *queue)
{
    if (pthread_mutex_lock(&(queue->q_lock)) != 0) { 
        return NULL;
    }

    while (queue->q_size == 0) {
        pthread_cond_wait(&(queue->q_cond), &(queue->q_lock));
    }

    void* data;
    if ((data = list_remove_head(queue->elems)) == NULL) {
        return NULL;
    }

    queue->q_size--; 

    if (pthread_mutex_unlock(&(queue->q_lock)) != 0) { 
        return NULL;
    }
    
    return data;
}