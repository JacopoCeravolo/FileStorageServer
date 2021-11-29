#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "server/server_config.h"
#include "server/worker.h"
#include "server/request_handlers.h"
#include "utils/protocol.h"
#include "utils/logger.h"

void 
worker_thread(void* args)
{
   assert(args);

   int pipe_fd = ((worker_arg_t*)args)->pipe_fd;
   int worker_id = ((worker_arg_t*)args)->worker_id;
   concurrent_queue_t *requests = ((worker_arg_t*)args)->requests;
   char worker_name[32];
   sprintf(worker_name, "WORKER %d", worker_id);

   free(args);

    do {

        if (worker_exit_signal == 1) { break; continue;}
        
        int client_fd;
  
        client_fd = (int)concurrent_queue_get(requests);
        
        if (client_fd == -1) {break; continue; }
        
        request_t *request;
        request = recv_request(client_fd);
        if (request == NULL) {
          log_error("request could not be received: %s\n", strerror(errno));
          close(client_fd);
          client_fd = -1;
          goto _write_pipe;
        }

        switch (request->type) {

            case OPEN_CONNECTION: {
                int status = 0;
                status = open_connection_handler(client_fd);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                // log_info("client %d connection opened\n", client_fd);
                break;
            }
    
            case CLOSE_CONNECTION: {
                int status = 0;
                status = close_connection_handler(client_fd);
                if (status != 0) log_error("an error occurred when closing client %d connection\n", client_fd);
                // log_info("client %d connection closed\n", client_fd);
                close(client_fd);
                client_fd = -1;
                break;
            }
            
            case OPEN_FILE: {
                list_t *expelled_files = list_create(NULL, free_file, NULL);
                int status = open_file_handler(client_fd, request, expelled_files);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                list_destroy(expelled_files);
                break;
            }
            
            case CLOSE_FILE: {
                int status = close_file_handler(client_fd, request);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                break;
            }
            
            case WRITE_FILE: {
                list_t *expelled_files = list_create(NULL, free_file, NULL);
                int status = write_file_handler(client_fd, request, expelled_files);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                list_destroy(expelled_files);
                break;
            }

            case READ_FILE: {
                void *read_buffer = NULL;
                size_t buffer_size = 0;
                int status = read_file_handler(client_fd, request, &read_buffer, &buffer_size);
                /* if (status == SUCCESS && ((buffer_size < 0) || (read_buffer == NULL))) {
                    send_response(client_fd, INTERNAL_ERROR, status_message[INTERNAL_ERROR], "", 0, NULL);
                } */
                send_response(client_fd, status, status_message[status], "", buffer_size, read_buffer);
                if (read_buffer) free(read_buffer);
                break;
            }

            case READ_N_FILES: {
                log_info("READ_N_FILES has not been implemented yet\n");
                break;
            }

            case REMOVE_FILE: { 
                int status = remove_file_handler(client_fd, request);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                break;
            }

            case LOCK_FILE: {
                int status = lock_file_handler(client_fd, request);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                break;
            }

            case UNLOCK_FILE: {
                int status = unlock_file_handler(client_fd, request);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                break;
            }
        
        } 

_write_pipe:  
        write(pipe_fd, &client_fd, sizeof(int));
        free_request(request);

    } while (worker_exit_signal == 0);

    pthread_exit(0);
}
