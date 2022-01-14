#define EXTERN
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

#include "server/server_config.h"
#include "server/lock_manager.h"
#include "server/signal_handler.h"
#include "server/worker.h"

#define LOG_LVL      LOG_DEBUG
#define MAX_BACKLOG  2000000000

server_config_t         server_config;

storage_t               *storage;

server_status_t         *server_status;
pthread_mutex_t         server_status_mtx = PTHREAD_MUTEX_INITIALIZER;

list_t                  *request_queue;
pthread_mutex_t         request_queue_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t          request_queue_notempty = PTHREAD_COND_INITIALIZER;

FILE                    *storage_file;
FILE                    *log_file;
pthread_mutex_t         log_file_mtx = PTHREAD_MUTEX_INITIALIZER;

pthread_t               *worker_tids;
pthread_t               *sig_handler_tid;
pthread_t               *lock_handler_tid;

volatile sig_atomic_t   accept_connection;
volatile sig_atomic_t   shutdown_now;

int
start_worker_threads(int *mw_pipe);

int
shutdown_all_threads();

int
parse_configuration_file(const char *file_name)
{
   char *line = NULL;
   size_t len = 0;
   ssize_t read;
   FILE *config_file = fopen(file_name, "r+");
   if (config_file == NULL) return -1;

   while ((read = getline(&line, &len, config_file)) != -1) {

      char *parameter = strtok(line, "=");

      if (strcmp(parameter, "N_WORKERS") == 0) {
         char *tmp_str = strtok(NULL, "\n");
         int n_workers = atoi(tmp_str);
         server_config.no_of_workers = n_workers;
      }

      if (strcmp(parameter, "MAX_SIZE") == 0) {
         char *tmp_str = strtok(NULL, "\n");
         int max_size = atoi(tmp_str);
         server_config.max_size = max_size;
      }

      if (strcmp(parameter, "MAX_FILES") == 0) {
         char *tmp_str = strtok(NULL, "\n");
         int max_files = atoi(tmp_str);
         server_config.max_files = max_files;
      }

      if (strcmp(parameter, "SOCKET_PATH") == 0) {
         char *socket_path = strtok(NULL, "\n");
         // remove if present " " from socket path
         server_config.socket_path = calloc(1, strlen(socket_path) + 1);
         strcpy(server_config.socket_path, socket_path);
      }

      if (strcmp(parameter, "LOG_FILE") == 0) {
         char *log_path = strtok(NULL, "\n");
         // remove if present " " from socket path
         server_config.log_file = calloc(1, strlen(log_path) + 1);
         strcpy(server_config.log_file, log_path);
      }
   }

   free(line);
   fclose(config_file);
   return 0;
}

int
main(int argc, char const *argv[])
{

   if ( argc < 2 ) {
      printf("error, need at least one argument\n");
   }


   /* Read configuration file */
   // check validity of file
   if (parse_configuration_file(argv[1]) != 0) {
      fprintf(stderr, "Could not parse configuration file\n");
      exit(EXIT_FAILURE);
   }


   /* Init */
   int ret = 0;
   accept_connection = 1;
   shutdown_now = 0;
   server_status = malloc(sizeof(server_status_t));
   server_status->max_connections = 0;
   server_status->current_connections = 0;
   server_status->max_no_files_reached = 0;
   server_status->max_size_reached = 0;

   
   /* Initialize log file */
   if (server_config.log_file != NULL) {
      log_init(server_config.log_file);
   } else {
      log_init(NULL);
   }

   set_log_level(LOG_LVL);


   /* Max fd for select */
   int fd_max = -1;

   /* Install signal handler */
   int *signal_pipe = calloc(2, sizeof(int));
   if ( signal_pipe == NULL ) {
      log_fatal("Could not allocate signal handler pipe: %s\n", strerror(errno));
      return -1; 
   }

   if ( pipe(signal_pipe) == -1 ) {
	   log_fatal("Could not create signal handler pipe: %s\n", strerror(errno));
      free(signal_pipe);
      return -1;
   }

   fd_max = signal_pipe[0];
   sig_handler_tid = calloc(1, sizeof(pthread_t));
   
   if ( install_signal_handler(signal_pipe, sig_handler_tid) != 0 ) {
      log_fatal("Could not install signal handler: %s\n", strerror(errno));
      free(signal_pipe);
      return -1;
   }

   /* Initialize request queue */
   request_queue = list_create(int_compare, NULL, print_int); // ugly af
   if ( request_queue == NULL ) {
      log_fatal("Could not initialize request queue: %s\n", strerror(errno));
      free(signal_pipe);
      return -1;
   }

   /* Initialize storage*/
   storage = storage_create(server_config.max_size, server_config.max_files);
   if ( storage == NULL ) {
      log_error("Could not initialize storage\n");
      ret = -1;
      goto _server_exit1;
   }

   /* Starts lock manager */
   int lock_manager_pipe[2];
   if ( pipe(lock_manager_pipe) != 0 ) {
      log_fatal("Could not create lock manager pipe: %s\n", strerror(errno));
      ret = -1;
      goto _server_exit1;
   }

   if ( lock_manager_pipe[0] > fd_max ) {
      fd_max = lock_manager_pipe[0];
   }

   lock_handler_tid = calloc(1, sizeof(pthread_t));

   if ( setup_lock_manager(lock_manager_pipe, lock_handler_tid) != 0 ) {
      log_fatal("Could not setup lock manager: %s\n", strerror(errno));
      ret = -1;
      free(lock_handler_tid);
      goto _server_exit1;
   }


   /* Creates and starts workers */
   int mw_pipe[2];

   if ( pipe(mw_pipe) != 0 ) {
      log_fatal("Could not create master - worker pipe: %s\n", strerror(errno));
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
      log_fatal("Could not start worker threads: %s\n", strerror(errno));
      free(worker_tids);
      ret = -1;
      goto _server_exit1;
   }
   
   /* Opening the socket */
   long socket_fd;
   struct sockaddr_un serveraddr;

   unlink(server_config.socket_path);

   log_debug("Opening socket file %s\n", server_config.socket_path);

   if ( (socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      log_fatal("Cloud not initialize socket: %s\n", strerror(errno));
      ret = -1;
      goto _server_exit2;

   }

   if (socket_fd > fd_max) fd_max = socket_fd;

   memset(&serveraddr, 0, sizeof(serveraddr));
   serveraddr.sun_family = AF_UNIX;
   strcpy(serveraddr.sun_path, server_config.socket_path);

   if ( bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0 ) {
      log_fatal("Cloud not bind socket: %s\n", strerror(errno));
      ret = -1;
      goto _server_exit2;
   }

   if ( listen(socket_fd, MAX_BACKLOG) != 0 ) {
      log_fatal("Cloud not listen on socket: %s\n", strerror(errno));
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
   storage_file = fopen("../logs/storage.txt", "w+");


   /* Start accepting requests */


   // fprintf(stdout, "****** SERVER STARTED ******\n");

   while (shutdown_now == 0) {

      rdset = set;

      if ( select(fd_max + 1, &rdset, NULL, NULL, NULL) == -1 && errno != EINTR) {
	         log_fatal("Fatal error when selecting file descriptor: %s\n", strerror(errno));
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

            long client_fd;

		      if ( (fd == socket_fd) && FD_ISSET(socket_fd, &set) 
                  && accept_connection == 1 ) { // new client
               
               if ( (client_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) < 0) {
                  log_fatal("Could not accept new client connection: %s\n", strerror(errno));
                  continue;
               }

               FD_SET(client_fd, &set);

               if (client_fd > fd_max) fd_max = client_fd;

               lock_return((&server_status_mtx), -1); 
               server_status->current_connections++;
               if (server_status->current_connections > server_status->max_connections) {
                  server_status->max_connections = server_status->current_connections;
               }
               unlock_return((&server_status_mtx), -1);

            } else { // new request from already connected client

               int *tmp_fd = malloc(sizeof(int));
               *tmp_fd = fd;
      
               lock_return((&request_queue_mtx), -1);
               if ( list_insert_tail(request_queue, (void*)tmp_fd) != 0 ) {
                  log_error("Could not enqueue new client request\n");
               }

               cond_signal_return(&(request_queue_notempty), -1);

               unlock_return((&request_queue_mtx), -1);

               FD_CLR(fd, &set);
               if ( fd == fd_max ) {
                  fd_max--;
               }
            }
         }

         if ( FD_ISSET(fd, &rdset) && FD_ISSET(mw_pipe[0], &rdset) ) {
            
            int new_fd;

			   if( (read(mw_pipe[0], &new_fd, sizeof(int))) == -1 ) {
			   	log_fatal("Could not read on master - worker pipe: %s\n", strerror(errno));
			   	continue; // Should probably fail
			   } else {
               
			   	if( new_fd != -1 ){ // reinsert fd in listen set
			   		
                  FD_SET(new_fd, &set);
			         if( new_fd > fd_max ) fd_max = new_fd;
			   	}
			   }
		   } 
      }

      lock_return((&server_status_mtx), -1); 
      if ( (accept_connection == 0) && (server_status->current_connections == 0) ) {
         shutdown_now = 1;
         unlock_return((&server_status_mtx), -1);
         continue;
      }
      unlock_return((&server_status_mtx), -1);

   }

   // fprintf(stdout, "****** SERVER CLOSING ******\n");

   /* Joining threads */
   if ( shutdown_all_threads() != 0 ) {
      ret = -1;
   }

   log_info("[GENERAL INFO] Maximum number of connections: %d\n", server_status->max_connections);

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
   free(server_status);
   close(signal_pipe[0]); 
   free(signal_pipe);
   storage_destroy(storage);
   fclose(storage_file);
   free(server_config.log_file);
   free(server_config.socket_path);
   close_log();
   list_destroy(request_queue); 
   
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
      // worker_args->requests = request_queue;

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

   for (int i = 0; i < server_config.no_of_workers; i++) {
      
      int *terminate = malloc(sizeof(int));
      *terminate = -1;
      
      lock_return((&request_queue_mtx), -1);
      if ( list_insert_tail(request_queue, (void*)terminate) != 0 ) {
         log_error("Could not send thread termination signal\n");
      }

      cond_signal_return((&request_queue_notempty), -1);

      unlock_return((&request_queue_mtx), -1);
   }

   for (int i = 0; i < server_config.no_of_workers; i++) {
      if ( (res = pthread_join(worker_tids[i], NULL)) != 0 ) {
         log_error("Could not join worker thread %d\n", i);
      } 
   }
   return res;
}
