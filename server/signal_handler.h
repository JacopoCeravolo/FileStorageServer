#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

#include "server_config.h"

/**
 * Signal handler arguments
 */
typedef struct {
    sigset_t     *set;           
    int          *signal_pipe;  
} sig_handler_arg_t;

/**
 * Signal handler thread
 */
void* 
sig_handler_thread(void *arg);

/**
 * Installation of signal handler
 */
int 
install_signal_handler(int *signal_pipe,  pthread_t *sig_handler_id);

#endif