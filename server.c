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

#include "concurrent_queue.h"
#include "protocol.h"
#include "storage.h"
#include "server_config.h"
#include "utilities.h"
#include "worker.h"


storage_t *storage;
FILE *logfile;

void
cleanup()
{
   unlink(DEFAULT_SOCKET_PATH);
}

void 
int_handler(int dummy) {
   printf("\nIn Signal Handler\n");
   storage_dump(storage, logfile);
   fclose(logfile);
   printf("Exiting...\n");
   exit(EXIT_SUCCESS);
}

int
main(int argc, char const *argv[])
{
   /* Exit cleanup and basic signal handling */

   atexit(cleanup);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGINT, int_handler);

   /* Opens logfile */

   logfile = fopen("server_log.txt", "w+");

   /* Initialize storage with size 16384 and 10 files */

   storage = storage_create(16384, 10);
   if (storage == NULL) {
      printf("FATAL ERROR, storage could not be initialized\n");
      exit(EXIT_FAILURE);
   }

   printf("Storage created\n");

   /* Opening the socket */

   int socket_fd, rc;
   struct sockaddr_un serveraddr;

   unlink(DEFAULT_SOCKET_PATH);

   socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (socket_fd < 0) {
      perror("socket() failed");
      exit(EXIT_FAILURE);
   }

   memset(&serveraddr, 0, sizeof(serveraddr));
   serveraddr.sun_family = AF_UNIX;
   strcpy(serveraddr.sun_path, DEFAULT_SOCKET_PATH);

   rc = bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
   if (rc < 0) {
      perror("bind() failed");
      return -1;
   }

   rc = listen(socket_fd, 50);
   if (rc< 0) {
      perror("listen() failed");
      return -1;
   }

   printf("Socket ready\n");

   /* Setting pipes and file descriptor set */

   int    mw_pipe[2];   // master-worker pipe
   fd_set set;          // set of opened file descriptors
   fd_set rdset;        // set of descriptors ready on read


   FD_ZERO(&set);
   FD_ZERO(&rdset);

   if (pipe(mw_pipe) != 0) {
      perror("pipe()");
      return -1;
   }

   FD_SET(socket_fd, &set);
   FD_SET(mw_pipe[0], &set);

   int fd_max = 0;
   if (socket_fd > fd_max) fd_max = socket_fd;
   if (mw_pipe[0] > fd_max) fd_max = mw_pipe[0];

   /* Initializing dispatcher->worker queue */

   concurrent_queue_t *requests_queue;
   requests_queue = concurrent_queue_create(NULL, NULL, NULL);
   if (requests_queue == NULL) {
      printf("FATAL ERROR, dispatcher queue could not be initialized\n");
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
         printf("could not create thread\n");
         exit(EXIT_FAILURE);
      }
   }

   /* Start accepting requests */

   printf("Server can now accept connections\n");
   while (1) {
      
      rdset = set;
      if (select(fd_max+1, &rdset, NULL, NULL, NULL) == -1 && errno != EINTR) {
	       perror("select");
	       exit(-1);
	   }

      for(int fd = 0; fd <= fd_max; fd++) {

         if (FD_ISSET(fd, &rdset) && (fd != mw_pipe[0])) {
            int client_fd;
		      if (fd == socket_fd && FD_ISSET(socket_fd, &set)) { // new client
               
               client_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
               if (client_fd < 0) {
                  perror("accept() failed");
                  return -1;
               }

               printf("Accepted new connection from client %d\n", client_fd);
               FD_SET(client_fd, &set);
               if (client_fd > fd_max) fd_max = client_fd;

            } else { // new request from already connected client
               void *tmp_fd;
               tmp_fd = (void*)fd;
               if (concurrent_queue_put(requests_queue, tmp_fd) != 0) {
                  printf("ERROR, could not add fd %d to queue\n", fd);
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
			   	perror("read_pipe");
			   	return -1;
			   } else{
               printf("red client fd: %d\n", new_fd);
			   	if( new_fd != -1 ){ // reinserisco il fd tra quelli da ascoltare
			   		// FD_SET(new_fd, &set);
			         // if( new_fd > fd_max ) fd_max = new_fd;
			   	}
			   }
		   }
      }
   }

   
   storage_destroy(storage);
   concurrent_queue_destroy(requests_queue);
   close(socket_fd);
   return 0;
}
