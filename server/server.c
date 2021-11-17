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

#include "utils/concurrent_queue.h"
#include "utils/protocol.h"
#include "utils/utilities.h"
#include "utils/logger.h"
#include "server/storage.h"
#include "server/server_config.h"
#include "server/worker.h"


storage_t *storage;
FILE *storage_file;

void
cleanup()
{
   unlink(DEFAULT_SOCKET_PATH);
}

void 
int_handler(int dummy) {
   printf("\nIn Interrupted Signal Handler\n");
   storage_dump(storage, storage_file);
   fclose(storage_file);
   storage_destroy(storage);
   printf("Exiting...\n");
   exit(EXIT_SUCCESS);
}

void
seg_handler(int dummy) 
{
   printf("\nIn Segmentation Fault Signal Handler. ERRNO: %s\n", strerror(errno));
   storage_dump(storage, storage_file);
   fclose(storage_file);
   storage_destroy(storage);
   printf("Exiting...\n");
   exit(EXIT_SUCCESS);
}
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

   atexit(cleanup);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGINT, int_handler);
   signal(SIGSEGV, seg_handler);

   /* Opens storage_file */

   storage_file = fopen("/Users/jacopoceravolo/Desktop/FileStorageServer/logs/storage.txt", "w+");

   /* Initialize storage with size 16384 and 10 files */

   storage = storage_create(100000000, 2);
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

   rc = listen(socket_fd, 50);
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

   int fd_max = 0;
   if (socket_fd > fd_max) fd_max = socket_fd;
   if (mw_pipe[0] > fd_max) fd_max = mw_pipe[0];

   /* Initializing dispatcher->worker queue */

   concurrent_queue_t *requests_queue;
   requests_queue = concurrent_queue_create(int_compare, NULL, print_int);
   if (requests_queue == NULL) {
      log_fatal("dispatcher queue could not be initialized\n");
      exit(EXIT_FAILURE);
   }

   /* Starting all workers */

   pthread_t *workers = malloc(sizeof(pthread_t) * 5);

   for (int id = 0; id < 5; id++) {
      worker_arg_t *worker_args = malloc(sizeof(worker_arg_t));
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
   while (1) {
      
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

               log_debug("new connection from client %d\n", client_fd);
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
      }
   }

   
   concurrent_queue_destroy(requests_queue);
   close(socket_fd);
   return 0;
}
