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

#include "server/server_config.h"
// #include "server/lock_manager.h"
#include "server/signal_handler.h"
#include "server/worker.h"

#define N_THREADS    1
#define MAX_SIZE     128000000
#define MAX_FILES    1000
#define MAX_BACKLOG  200


server_config_t      server_config;
server_mode_t        server_status;
storage_t            *storage;
concurrent_queue_t   *requests_queue;
hash_map_t           *connected_clients;
hash_map_t           *waiting_on_lock;

FILE *storage_file;

pthread_t            *worker_tids;
pthread_t            *sig_handler_tid;
pthread_t            *lock_handler_tid;

volatile sig_atomic_t    accept_connection;
volatile sig_atomic_t    shutdown_now;

int
start_worker_threads(int *mw_pipe);

int
shutdown_all_threads();

int
main(int argc, char const *argv[])
{
   /* Init */
   int ret = 0;
   accept_connection = 1;
   shutdown_now = 0;

   /* Configuration of the server */
   server_config.no_of_workers = 1;
   strcpy(server_config.socket_path, DEFAULT_SOCKET_PATH);
   server_status = OPEN;

   /* Max fd for select */
   int fd_max = -1;

   /* Initialize log file */
   // log_init("/Users/jacopoceravolo/Desktop/storage-server/logs/server.log");
   log_init(NULL);
   set_log_level(LOG_INFO);

   /* Install signal handler */
   int *signal_pipe = calloc(2, sizeof(int));
   if ( signal_pipe == NULL ) {
      log_fatal("Could not allocate signal handler pipe\n");
      return -1; 
   }

   if ( pipe(signal_pipe) == -1 ) {
	   log_fatal("Could not create signal handler pipe\n");
      free(signal_pipe);
      return -1;
   }

   fd_max = signal_pipe[0];
   sig_handler_tid = calloc(1, sizeof(pthread_t));
   
   if ( install_signal_handler(signal_pipe, sig_handler_tid) != 0 ) {
      log_fatal("Could not install signal handler\n");
      free(signal_pipe);
      return -1;
   }

   /* Initialize request queue */
   requests_queue = concurrent_queue_create(int_compare, NULL, print_int); // ugly af
   if ( requests_queue == NULL ) {
      log_fatal("Could not initialize request queue\n");
      free(signal_pipe);
      return -1;
   }

   /* Initialize connected clients hashtable */
   connected_clients = hash_map_create(MAX_BACKLOG * 2, int_hash, int_compare, NULL, list_destroy);
   if ( connected_clients == NULL ) {
      log_fatal("Could not initialize connected clients hashtable\n");
      free(signal_pipe);
      return -1;
   }

   /* Initialize storage*/
   storage = storage_create(MAX_SIZE, MAX_FILES);
   if ( storage == NULL ) {
      log_error("Could not initialize storage\n");
      ret = -1;
      goto _server_exit1;
   }

   /* Starts lock manager */
   int lock_manager_pipe[2];
   if ( pipe(lock_manager_pipe) != 0 ) {
      log_fatal("Could not create lock manager pipe\n");
      ret = -1;
      goto _server_exit1;
   }

   if ( lock_manager_pipe[0] > fd_max ) {
      fd_max = lock_manager_pipe[0];
   }

   lock_handler_tid = calloc(1, sizeof(pthread_t));

   if ( setup_lock_manager(lock_manager_pipe, lock_handler_tid) != 0 ) {
      log_fatal("Could not setup lock manager\n");
      ret = -1;
      free(lock_handler_tid);
      goto _server_exit1;
   }


   /* Creates and starts workers */
   int mw_pipe[2];

   if ( pipe(mw_pipe) != 0 ) {
      log_fatal("Could not create master - worker pipe\n");
      ret = -1;
      goto _server_exit1;
   }

   if ( mw_pipe[0] > fd_max ) {
      fd_max = mw_pipe[0];
   }

   worker_tids = calloc(server_config.no_of_workers, sizeof(pthread_t));
   if ( worker_tids == NULL ) {
      ret = -1;
      goto _server_exit1;
   }

   if ( start_worker_threads(mw_pipe) != 0 ) {
      log_fatal("Could not start worker threads\n");
      free(worker_tids);
      ret = -1;
      goto _server_exit1;
   }
   
   /* Opening the socket */
   int socket_fd, res;
   struct sockaddr_un serveraddr;

   unlink(server_config.socket_path);

   if ( (socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      log_fatal("Cloud not initialize socket\n");
      ret = -1;
      goto _server_exit2;

   }

   if (socket_fd > fd_max) fd_max = socket_fd;

   memset(&serveraddr, 0, sizeof(serveraddr));
   serveraddr.sun_family = AF_UNIX;
   strcpy(serveraddr.sun_path, server_config.socket_path);

   if ( bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0 ) {
      log_fatal("Cloud not bind socket\n");
      ret = -1;
      goto _server_exit2;
   }

   if ( listen(socket_fd, MAX_BACKLOG) != 0 ) {
      log_fatal("Cloud not listen on socket\n");
      ret = -1;
      goto _server_exit2;
   }

   /* Setting up file descriptors set */
   fd_set set;         
   fd_set rdset;  

   FD_ZERO(&set);
   FD_ZERO(&rdset); 

   FD_SET(signal_pipe[0], &set);
   FD_SET(mw_pipe[0], &set);
   FD_SET(socket_fd, &set);
        

   
   /* Opens storage_file */
   storage_file = fopen("/Users/jacopoceravolo/Desktop/storage-server/logs/storage.txt", "w+");


   /* Start accepting requests */


   fprintf(stdout, "****** SERVER STARTED ******\n");

   while (shutdown_now == 0) {
      
      rdset = set;

      if ( select(fd_max + 1, &rdset, NULL, NULL, NULL) == -1 && errno != EINTR) {
	         log_fatal("Fatal error when selecting file descriptor\n");
            ret = -1;
	         goto _server_exit2;
	   }

      if ( FD_ISSET(signal_pipe[0], &rdset) ) {

            FD_CLR(signal_pipe[0], &set);
            if ( signal_pipe[0] == fd_max ) {
               fd_max--;
            }

            FD_CLR(socket_fd, &set);
            if ( socket_fd == fd_max ) {
               fd_max--;
            }
            continue;
      }

      for(int fd = 0; fd <= fd_max; fd++) {

         if ( FD_ISSET(fd, &rdset) && (fd != mw_pipe[0]) ) {

            int client_fd;

		      if ( (fd == socket_fd) && FD_ISSET(socket_fd, &set) 
                  && accept_connection == 1 ) { // new client
               
               if ( (client_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) < 0) {
                  log_fatal("Could not accept new client connection\n");
                  continue;
               }

               FD_SET(client_fd, &set);

               if (client_fd > fd_max) fd_max = client_fd;

            } else { // new request from already connected client

               void *tmp_fd = (void*)fd;
         
               if ( concurrent_queue_put(requests_queue, tmp_fd) != 0 ) {
                  log_error("Could not enqueue new client request\n");
               }

               FD_CLR(fd, &set);
               if ( fd == fd_max ) {
                  fd_max--;
               }
            }
         }

         if ( FD_ISSET(fd, &rdset) && FD_ISSET(mw_pipe[0], &rdset) ) {
            
            int new_fd;

			   if( (read(mw_pipe[0], &new_fd, sizeof(int))) == -1 ) {
			   	log_fatal("Could not read on master - worker pipe\n");
			   	continue; // Should probably fail
			   } else {
               
			   	if( new_fd != -1 ){ // reinsert fd in listen set
			   		
                  FD_SET(new_fd, &set);
			         if( new_fd > fd_max ) fd_max = new_fd;
			   	}
			   }
		   } 
      }
   }

   fprintf(stdout, "****** SERVER CLOSING ******\n");

   /* Joining threads */
   if ( shutdown_all_threads() != 0 ) {
      ret = -1;
   }
   

_server_exit2:
   storage_dump(storage, storage_file);
   close(socket_fd);
   unlink(server_config.socket_path);
   free(sig_handler_tid);
   free(lock_handler_tid);
   free(worker_tids);
   close(mw_pipe[0]);
   close(mw_pipe[1]);
_server_exit1:
   close(signal_pipe[0]); 
   free(signal_pipe);
   storage_destroy(storage);
   hash_map_destroy(connected_clients);
   fclose(storage_file);
   close_log();
   // concurrent_queue_destroy(request_queue); // leads to SIGSEV
   
   return ret;
}


int
start_worker_threads(int *mw_pipe)
{
   for (int i = 0; i < server_config.no_of_workers; i++) {

      worker_arg_t *worker_args = calloc(1, sizeof(worker_arg_t));
      if ( worker_args == NULL ) {
         log_fatal("Could not allocate worker thread arguments\n");
         return -1;
      }

      worker_args->worker_id = i;
      worker_args->pipe_fd = mw_pipe[1];
      worker_args->requests = requests_queue;

      if ( pthread_create(&worker_tids[i], NULL, worker_thread, (void*)worker_args ) != 0) {
         log_fatal("Could not create worker thread\n");
         return -1;
      }
   }

   return 0;
}

int
shutdown_all_threads() 
{
   int res;

   if ( (res = pthread_join(*sig_handler_tid, NULL)) != 0 ) {
         log_error("Could not join signal handler thread\n");
   } 

   if ( (res = pthread_join(*lock_handler_tid, NULL)) != 0 ) {
         log_error("Could not join signal handler thread\n");
   } 

   /* This all should be moved to a separate pipe between master and workers */
   for (int i = 0; i < server_config.no_of_workers; i++) {
      
      int terminate = -1;
      void *signal_worker = (void*)terminate;
      
      if ( concurrent_queue_put(requests_queue, signal_worker) != 0 ) {
         log_error("Could not send thread termination signal\n");
      }
   }

   for (int i = 0; i < server_config.no_of_workers; i++) {
      if ( (res = pthread_join(worker_tids[i], NULL)) != 0 ) {
         log_error("Could not join worker thread %d\n", i);
      } 
   }
   return res;
}