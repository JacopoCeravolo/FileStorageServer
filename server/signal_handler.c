#include "signal_handler.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "utils/logger.h"

// funzione eseguita dal signal handler thread
void* 
sig_handler_thread(void *arg) {
    sigset_t *set = ((sig_handler_arg_t*)arg)->set;
    int *signal_pipe = ((sig_handler_arg_t*)arg)->signal_pipe;

    free(arg);

    while(1) {

	    int sig_num;
	    int res = sigwait(set, &sig_num);
	    if (res != 0) {
	        errno = res;
	        perror("FATAL ERROR 'sigwait'");
	        sig_num = SIGINT;
	    }

	    switch(sig_num) {
	        case SIGINT:
	        case SIGQUIT: 
                log_info("Received immediate shutdown signal\n");
                server_status = SHUTDOWN_NOW;

                close(signal_pipe[1]); 
                signal_pipe[1] = -1;
                free(set);
                return NULL;

	        case SIGHUP:
	            log_info("Received gracefull shutdown signal\n");
                server_status = SHUTDOWN;

                close(signal_pipe[1]); 
                signal_pipe[1] = -1;
                free(set);
                return NULL;

	        default:  ; 
	    }
    }

    return NULL;	   
}

int 
install_signal_handler(int *signal_pipe, pthread_t *sig_handler_id)
{
    sigset_t *mask;
    struct sigaction s;
    sig_handler_arg_t *handler_args;

    mask = malloc(sizeof(sigset_t));
    if (mask == NULL) {
        errno = ENOMEM; 
        return -1;
    }

    sigemptyset(mask);
    sigaddset(mask, SIGINT); 
    sigaddset(mask, SIGQUIT);
    sigaddset(mask, SIGHUP);

    if (pthread_sigmask(SIG_BLOCK, mask, NULL) != 0) {
	   log_fatal("could not set signal mask\n");
	   return -1;
    }

   memset(&s,0,sizeof(s));    
   s.sa_handler = SIG_IGN;

    if ((sigaction(SIGPIPE, &s, NULL)) == -1 ) {   
	    log_fatal("error when setting SIGPIPE handler\n");
        return -1;
    } 

    handler_args = malloc(sizeof(sig_handler_arg_t));
    if (handler_args == NULL) {
        errno = ENOMEM;
        return -1;
    }

    handler_args->set = mask;
    handler_args->signal_pipe = signal_pipe;

    if( pthread_create(sig_handler_id, NULL, &sig_handler_thread, (void*)handler_args) != 0 ) return -1;
    /* if( pthread_detach(sig_handler_id) != 0 ) return -1; */

    return 0;
}
