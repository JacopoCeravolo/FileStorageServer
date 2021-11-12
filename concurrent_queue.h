#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <pthread.h>
#include <stdio.h>
#include "linked_list.h"

typedef struct _concurrent_queue_t {

    list_t          *elems;
    size_t          q_size;

    pthread_mutex_t q_lock;
    pthread_cond_t  q_cond;

} concurrent_queue_t;

concurrent_queue_t*
concurrent_queue_create(bool cmp(void*, void*), void free_fun(void*), void print(void*, FILE*));

int
concurrent_queue_destroy(concurrent_queue_t *queue);

int
concurrent_queue_put(concurrent_queue_t *queue, void* data);

void*
concurrent_queue_get(concurrent_queue_t *queue);

#endif