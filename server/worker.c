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

    int error;
    while (exit_signal == 0) {

        int client_fd;
  
        client_fd = (int)concurrent_queue_get(requests);
        

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
    
                log_info("client %d opening connection\n", client_fd);

                int status = 0;
                list_t *files_opened_by_client; 
                files_opened_by_client = list_create(string_compare, NULL, string_print);
                if (files_opened_by_client == NULL) {
                    log_fatal("could not create opened file list: %s\n", strerror(errno));
                    status = INTERNAL_ERROR;
                }

                if (hash_map_insert(storage->opened_files, client_fd, files_opened_by_client) != 0) {
                    log_fatal("could not add opened file list: %s\n", strerror(errno));
                    status = INTERNAL_ERROR;
                }

                send_response(client_fd, status, status_message[status], request->resource_path, 0, NULL);
                break;
            }
    
            case CLOSE_CONNECTION: {
                int status = 0;
                status = close_connection_handler(client_fd);
                if (status != 0) log_error("an error occurred when closing client %d connection\n", client_fd);
                log_info("client %d connection closed\n", client_fd);
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

            case WRITE_DIRECTORY: {
                break;
            }

            case READ_FILE: {
                int status = read_file_handler(client_fd, request);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                break;
    
                break;
            }

            case READ_N_FILES: {
                break;
            }

            case DELETE_FILE: { // NOT WORKING
                int status = remove_file_handler(client_fd, request);
                send_response(client_fd, status, status_message[status], "", 0, NULL);
                break;
                break;
            }

            case LOCK_FILE: {
                break;
            }

            case UNLOCK_FILE: {
                break;
            }
        }     
_write_pipe:  
        write(pipe_fd, &client_fd, sizeof(int));
        free_request(request);
    }

    log_info("terminating\n", worker_name);
}
