#ifndef LOCK_MANAGER_H
#define LOCK_MANAGER_H

#include "server_config.h"

void*
lock_manager_thread(void* args);

int 
setup_lock_manager(int *lock_manager_pipe, pthread_t *lock_manager_id);

#endif