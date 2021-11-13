#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "server_config.h"
#include "worker.h"
#include "protocol.h"

void 
worker_thread(void* args)
{
   assert(args);

   int pipe_fd = ((worker_arg_t*)args)->pipe_fd;
   int worker_id = ((worker_arg_t*)args)->worker_id;
   concurrent_queue_t *requests = ((worker_arg_t*)args)->requests;
   
   free(args);

   while (1) {

      int client_fd;

      client_fd = (int)concurrent_queue_get(requests);

      printf("Worker %d serving client %d\n", worker_id, client_fd);

      request_t *request;
      request = recv_request(client_fd);
      if (request == NULL) {
        printf("request could not be received, exiting\n");
        exit(EXIT_FAILURE);
      }

      switch (request->type) {

        case OPEN_CONNECTION: {
            // As for now new connections are always accepted
            printf("OPEN CONNECTION\n");
            if (send_response(client_fd, SUCCESS, status_message[SUCCESS], 0, NULL) != 0) {
                printf("Error when sending request, %s\n", strerror(errno));
            }
            break;
        }
        
        case CLOSE_CONNECTION: {
            // Basic end of connection, closes associated fd and writes -1 on pipe
            printf("CLOSE CONNECTION\n");
            close(client_fd);
            client_fd = -1;
            break;
        }
        case OPEN_FILE: {
            break;
        }
        case CLOSE_FILE: {
            break;
        }
        case WRITE_FILE: {
            printf("WRITE FILE\n");
            // TODO: concurrent access to storage
            if (request->body_size == 0 || request->body == NULL) {
                if (send_response(client_fd, MISSING_BODY, status_message[MISSING_BODY], 0, NULL) != 0) {
                    printf("Error when sending request, %s\n", strerror(errno));
                }
            }
                file_t *new_file;
                strcpy(new_file->path, request->resource_path);
                new_file->size = request->body_size;
                new_file->contents = malloc(new_file->size);
                memcpy(new_file->contents, request->body, new_file->size);
                if (storage_add_file(storage, new_file) != 0) {
                    // check for errors
                } else {
                    send_response(client_fd, SUCCESS, status_message[SUCCESS], 0, NULL);
                }
            break;
        }
        case WRITE_DIRECTORY: {
            break;
        }
        case READ_FILE: {
            break;
        }
        case READ_N_FILES: {
            break;
        }
        case DELETE_FILE: {
            break;
        }
        case LOCK_FILE: {
            break;
        }
        case UNLOCK_FILE: {
            break;
        }
      }

      write(pipe_fd, &client_fd, sizeof(int));

   }
}
