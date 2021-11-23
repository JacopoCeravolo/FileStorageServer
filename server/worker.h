#ifndef WORKER_H
#define WORKER_H

#include "utils/concurrent_queue.h"

typedef struct _worker_arg_t {
   concurrent_queue_t *requests;
   long *exit_signal;
   int pipe_fd;
   int worker_id;
} worker_arg_t;

void 
worker_thread(void* args);

#endif