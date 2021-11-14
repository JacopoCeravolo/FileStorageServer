#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "server/server_config.h"
#include "server/worker.h"
#include "utils/protocol.h"

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

        // printf("Worker %d serving client %d\n", worker_id, client_fd);

        request_t *request;
        request = recv_request(client_fd);
        if (request == NULL) {
          printf("request could not be received, exiting\n");
          exit(EXIT_FAILURE);
        }

        switch (request->type) {

            case OPEN_CONNECTION: {
                // As for now new connections are always accepted
                // WHAT ELSE? Check server state (active, gracefull or brutal)
    
                printf("OPEN CONNECTION\n");
                if (send_response(client_fd, SUCCESS, status_message[SUCCESS], 0, NULL) != 0) {
                    printf("Error when sending response, %s\n", strerror(errno));
                }
                break;
            }
    
            case CLOSE_CONNECTION: {
                // Basic end of connection, closes associated fd and writes -1 on pipe
                // WHAT ELSE? Close and unlock all open files by client
    
                printf("CLOSE CONNECTION\n");
                close(client_fd);
                client_fd = -1;
                break;
            }
            case OPEN_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components, check for availability and cache the file
    
                printf("OPEN FILE\n");
                if (storage_open_file(storage, client_fd, request->resource_path) != 0) {
                    printf("Could not open file, %s\n", strerror(errno));
                }
    
                send_response(client_fd, SUCCESS, status_message[SUCCESS], 0, NULL);
    
                break;
            }
            case CLOSE_FILE: {
                break;
            }
            case WRITE_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components, check for availability and cache the file
    
                printf("WRITE FILE\n");
    
                if (request->body_size == 0 || request->body == NULL) {
                    if (send_response(client_fd, MISSING_BODY, status_message[MISSING_BODY], 0, NULL) != 0) {
                        printf("Error when sending response, %s\n", strerror(errno));
                    }
                }

                // Probably should move this to storage
                list_t *files_opened_by = (list_t*)hash_map_get(storage->opened_files, client_fd);
                if (list_index_of(files_opened_by, request->resource_path) == -1) {
                    send_response(client_fd, UNAUTHORIZED, status_message[UNAUTHORIZED], 0, NULL);
                    break;
                }
                // ****

                file_t *new_file = malloc(sizeof(file_t));
                strcpy(new_file->path, request->resource_path);
                new_file->size = request->body_size;
                new_file->contents = malloc(new_file->size);
                memcpy(new_file->contents, request->body, new_file->size);
                if (storage_add_file(storage, new_file) != 0) {
                    // check for errors
                    send_response(client_fd, INTERNAL_ERROR, status_message[INTERNAL_ERROR], 0, NULL);
                } else {
                    send_response(client_fd, SUCCESS, status_message[SUCCESS], 0, NULL);
                }
    
    
                break;
            }
            case WRITE_DIRECTORY: {
                break;
            }
            case READ_FILE: {
                // TODO: concurrent access to storage
                printf("READ FILE\n");
    
                file_t *file;
    
                file = storage_get_file(storage, request->resource_path);
                if (file == NULL) {
                    send_response(client_fd, NOT_FOUND, status_message[NOT_FOUND], 0, NULL);
                } else {
                    /* reading_buffer = malloc(file->size);
                    memcpy(reading_buffer, file->contents, file->size); */
                    send_response(client_fd, SUCCESS, status_message[SUCCESS], file->size, file->contents);
                }
    
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
        // if (request) free_request(request);
    }
}
