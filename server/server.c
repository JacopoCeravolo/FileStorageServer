#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#define EXTERN

#include "utils/concurrent_queue.h"
#include "utils/protocol.h"
#include "utils/utilities.h"
#include "utils/logger.h"
#include "server/storage.h"
#include "server/server_config.h"
#include "server/worker.h"

#define N_THREADS    1
#define MAX_SIZE     128000000
#define MAX_FILES    1000
#define MAX_BACKLOG  200

FILE *storage_file;
concurrent_queue_t *requests_queue;
pthread_t workers[N_THREADS];

void
cleanup()
{
   unlink(DEFAULT_SOCKET_PATH);
}

typedef struct {
    sigset_t     *set;           /// set dei segnali da gestire (mascherati)
    int           signal_pipe;   /// descrittore di scrittura di una pipe senza nome
} sigHandler_t;


// funzione eseguita dal signal handler thread
static void *sigHandler(void *arg) {
    sigset_t *set = ((sigHandler_t*)arg)->set;
    int fd_pipe   = ((sigHandler_t*)arg)->signal_pipe;

    for( ;; ) {
	int sig;
	int r = sigwait(set, &sig);
	if (r != 0) {
	    errno = r;
	    perror("FATAL ERROR 'sigwait'");
	    return NULL;
	}

	switch(sig) {
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
	    log_info("received signal %s, exiting\n", (sig==SIGINT) ? "SIGINT": ((sig==SIGTERM)?"SIGTERM":"SIGQUIT") );
	    close(fd_pipe);  // notifico il listener thread della ricezione del segnale
	    return NULL;
	default:  ; 
	}
    }
    return NULL;	   
}

/* 
void 
int_handler(int dummy) {
   printf("\nIn Interrupted Signal Handler\n");
   storage_dump(storage, storage_file);
   fclose(storage_file);
   storage_destroy(storage);
   exit_signal = 1;
   concurrent_queue_destroy(requests_queue);
   sleep(3);
   printf("Shutting down thread\n");
   for (size_t i = 0; i < N_THREADS; i++) {
      pthread_join(workers[i], NULL);
   }
   printf("Exiting...\n");
   exit(EXIT_SUCCESS);
}
 */
/* void
seg_handler(int dummy) 
{
   printf("\nIn Segmentation Fault Signal Handler. ERRNO: %s\n", strerror(errno));
   storage_dump(storage, storage_file);
   fclose(storage_file);
   storage_destroy(storage);
   printf("Exiting...\n");
   exit(EXIT_SUCCESS);
} */

int
main(int argc, char const *argv[])
{

#ifdef DEBUG
   log_init("/Users/jacopoceravolo/Desktop/FileStorageServer/logs/server.log");
   set_log_level(LOG_DEBUG);
#else
   log_init(NULL);
#endif


   /* Exit cleanup and basic signal handling */

   sigset_t mask;
   sigemptyset(&mask);
   sigaddset(&mask, SIGINT); 
   sigaddset(&mask, SIGQUIT);
   sigaddset(&mask, SIGTERM);
    
   if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
	   log_fatal("could not set signal mask\n");
	   exit(EXIT_FAILURE);
   }

   // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket
   struct sigaction s;
   memset(&s,0,sizeof(s));    
   s.sa_handler=SIG_IGN;
   if ( (sigaction(SIGPIPE,&s,NULL) ) == -1 ) {   
	   log_fatal("error when ignoring SIGPIPE\n");
      exit(EXIT_FAILURE);
   } 

   int signal_pipe[2];
   if (pipe(signal_pipe)==-1) {
	   log_fatal("pipe failed\n");
      exit(EXIT_FAILURE);
   }

   pthread_t sighandler_thread;
   sigHandler_t handlerArgs = { &mask, signal_pipe[1] };
   
   if (pthread_create(&sighandler_thread, NULL, sigHandler, &handlerArgs) != 0) {
	   log_fatal("could not start signal handler\n");
	   exit(EXIT_FAILURE);
   }

   /* Opens storage_file */

   storage_file = fopen("/Users/jacopoceravolo/Desktop/FileStorageServer/logs/storage.txt", "w+");

   /* Initialize storage with size 16384 and 10 files */

   storage = storage_create(MAX_SIZE, MAX_FILES);
   if (storage == NULL) {
      log_error("storage could not be initialized\n");
      exit(EXIT_FAILURE);
   }

   /* Opening the socket */

   int socket_fd, rc;
   struct sockaddr_un serveraddr;

   unlink(DEFAULT_SOCKET_PATH);

   socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (socket_fd < 0) {
      log_fatal("socket() failed");
      exit(EXIT_FAILURE);
   }

   memset(&serveraddr, 0, sizeof(serveraddr));
   serveraddr.sun_family = AF_UNIX;
   strcpy(serveraddr.sun_path, DEFAULT_SOCKET_PATH);

   rc = bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
   if (rc < 0) {
      log_fatal("bind() failed");
      return -1;
   }

   rc = listen(socket_fd, MAX_BACKLOG);
   if (rc< 0) {
      log_fatal("listen() failed");
      return -1;
   }

   /* Setting pipes and file descriptor set */

   int    mw_pipe[2];   // master-worker pipe
   fd_set set;          // set of opened file descriptors
   fd_set rdset;        // set of descriptors ready on read


   FD_ZERO(&set);
   FD_ZERO(&rdset);

   if (pipe(mw_pipe) != 0) {
      log_fatal("pipe()");
      return -1;
   }

   FD_SET(socket_fd, &set);
   FD_SET(mw_pipe[0], &set);
   FD_SET(signal_pipe[0], &set);

   int fd_max = 0;
   if (socket_fd > fd_max) fd_max = socket_fd;
   if (mw_pipe[0] > fd_max) fd_max = mw_pipe[0];
   if (signal_pipe[0] > fd_max) fd_max = signal_pipe[0];

   /* Initializing dispatcher->worker queue */

   requests_queue = concurrent_queue_create(int_compare, NULL, print_int);
   if (requests_queue == NULL) {
      log_fatal("dispatcher queue could not be initialized\n");
      exit(EXIT_FAILURE);
   }

   /* Starting all workers */

   volatile long exit_signal = 0;

   for (int id = 0; id < N_THREADS; id++) {
      worker_arg_t *worker_args = calloc(1, sizeof(worker_arg_t));
      worker_args->exit_signal = (long)&exit_signal;
      worker_args->worker_id = id;
      worker_args->pipe_fd = mw_pipe[1];
      worker_args->requests = requests_queue;
      if (pthread_create(&workers[id], NULL, worker_thread, (void*)worker_args) != 0) {
         log_fatal("could not create thread\n");
         exit(EXIT_FAILURE);
      }
   }

   /* Start accepting requests */


   fprintf(stdout, "****** SERVER STARTED ******\n");
   while (exit_signal == 0) {
      
      rdset = set;
      if (select(fd_max+1, &rdset, NULL, NULL, NULL) == -1 && errno != EINTR) {
	       log_fatal("select");
	       exit(-1);
	   }

      for(int fd = 0; fd <= fd_max; fd++) {

         if (FD_ISSET(fd, &rdset) && (fd != mw_pipe[0])) {
            int client_fd;
		      if (fd == socket_fd && FD_ISSET(socket_fd, &set)) { // new client
               
               client_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
               if (client_fd < 0) {
                  log_fatal("accept() failed");
                  return -1;
               }
               FD_SET(client_fd, &set);
               if (client_fd > fd_max) fd_max = client_fd;

            } else { // new request from already connected client
               void *tmp_fd;
               tmp_fd = (void*)fd;
               if (concurrent_queue_put(requests_queue, tmp_fd) != 0) {
                  log_error("could not add fd %d to queue\n", fd);
               }

               FD_CLR(fd, &set);
               if (fd == fd_max) {
                  fd_max--;
               }
               
            }
         }

         if (FD_ISSET(fd, &rdset) && FD_ISSET(mw_pipe[0], &rdset)) {
            
            int new_fd;

			   if( (read(mw_pipe[0], &new_fd, sizeof(int))) == -1 ) {
			   	log_fatal("read_pipe");
			   	return -1;
			   } else{
			   	if( new_fd != -1 ){ // reinserisco il fd tra quelli da ascoltare
			   		FD_SET(new_fd, &set);
			         if( new_fd > fd_max ) fd_max = new_fd;
			   	}
			   }
		   }

         if (FD_ISSET(fd, &rdset) && fd == signal_pipe[0]) {
            exit_signal = 1;
            break;
         }
      }
   }

   pthread_join(sighandler_thread, NULL);
   log_info("Joined signal handler\n"); 
   for (int i = 0; i < N_THREADS; i++) {
      pthread_join(workers[i], NULL);
      log_info("Joined worker %d\n", i);
   }
   

   fprintf(stdout, "****** SERVER CLOSED ******\n");
   
   // concurrent_queue_destroy(requests_queue);
   close(socket_fd);
   return 0;
}
