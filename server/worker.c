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
    
                printf("OPEN CONNECTION [client %d]\n", client_fd);
                int status = 0;
                if (storage_add_client(storage, client_fd) != 0) status = INTERNAL_ERROR;

                if (send_response(client_fd, status, status_message[status], "", 0, NULL) != 0) {
                    printf("Error when sending response, %s\n", strerror(errno));
                }
                break;
            }
    
            case CLOSE_CONNECTION: {
                // Basic end of connection, closes associated fd and writes -1 on pipe
                // WHAT ELSE? Close and unlock all open files by client
    
                printf("CLOSE CONNECTION [client %d]\n", client_fd);
                if (storage_remove_client(storage, client_fd) != 0) {
                    printf("client %d was not removed from client table\n", client_fd);
                }
                close(client_fd);
                client_fd = -1;
                break;
            }
            case OPEN_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components, check for availability and cache the file
    
                printf("OPEN FILE [%s]\n", request->resource_path);
                if (storage_open_file(storage, client_fd, request->resource_path) != 0) {
                    printf("Could not open file, %s\n", strerror(errno));
                }
    
                send_response(client_fd, SUCCESS, status_message[SUCCESS], request->resource_path, 0, NULL);
    
                break;
            }
            case CLOSE_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components

                printf("CLOSE FILE [%s]\n", request->resource_path);
                int status = 0;
                if (storage_close_file(storage, client_fd, request->resource_path) != 0) {
                    switch (errno) {
                        case ENOMEM: status = INTERNAL_ERROR; break;
                        case ENOENT: status = NOT_FOUND; break;
                    }
                }

                send_response(client_fd, status, status_message[status], request->resource_path, 0, NULL);

                break;
            }
            case WRITE_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components, check for availability and cache the file
    
                printf("WRITE FILE [%s]\n", request->resource_path);
    
                /* Checks if the request contains any content */

                if (request->body_size == 0 || request->body == NULL) {
                    send_response(client_fd, MISSING_BODY, status_message[MISSING_BODY], "", 0, NULL);
                }

                /* Creates the file */

                file_t *new_file = malloc(sizeof(file_t));
                strcpy(new_file->path, request->resource_path);
                new_file->size = request->body_size;
                new_file->contents = malloc(new_file->size);
                memcpy(new_file->contents, request->body, new_file->size);

                /** Writes the file to storage unit, an uninitialized list
                 *  is passed to hold any expelled file
                 */

                int result;
                list_t *expelled_files = list_create(NULL, free_file, print_file);
                if (expelled_files == NULL) return -1;

                int status = 0;
                if ((result = storage_add_file(storage, client_fd, new_file, expelled_files)) > 0) {
                    if (list_is_empty(expelled_files)) {
                        send_response(client_fd, INTERNAL_ERROR, status_message[INTERNAL_ERROR], "", 0, NULL);
                    } else {
                        send_response(client_fd, FILES_EXPELLED, status_message[FILES_EXPELLED], "", sizeof(int), &result);

                        while (result > 0) {
                            file_t *expelled = (file_t*)list_remove_head(expelled_files);
                            send_response(client_fd, FILES_EXPELLED, status_message[FILES_EXPELLED], expelled->path, 
                                            expelled->size, expelled->contents);
                            result--;
                        }
                    }
                } else {
                    // error checking
                    switch (result) {
                        // case EINVAL: status = INTERNAL_ERROR; break; // Better invalid argument status?
                        case E_NOPERM: status = UNAUTHORIZED; break;
                        case E_TOOBIG: status = FILE_TOO_BIG; break;
                    }

                    send_response(client_fd, status, status_message[status], request->resource_path, 0, NULL);
                }
                break;
            }
            case WRITE_DIRECTORY: {
                break;
            }
            case READ_FILE: {
                // TODO: concurrent access to storage
                printf("READ FILE [%s]\n", request->resource_path);
    
                file_t *file;
    
                file = storage_get_file(storage, client_fd, request->resource_path);
                if (file == NULL) {
                    send_response(client_fd, NOT_FOUND, status_message[NOT_FOUND], request->resource_path, 0, NULL);
                } else {
                    /* reading_buffer = malloc(file->size);
                    memcpy(reading_buffer, file->contents, file->size); */
                    send_response(client_fd, SUCCESS, status_message[SUCCESS], file->path, file->size, file->contents);
                }
    
                break;
            }
            case READ_N_FILES: {
                break;
            }
            case DELETE_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components

                printf("DELETE FILE [%s]\n", request->resource_path);
                int status = 0;
                if (storage_remove_file(storage, request->resource_path) == NULL) {
                    switch (errno) {
                        // Will be more
                        case ENOENT: status = NOT_FOUND; break;
                    }
                }

                send_response(client_fd, status, status_message[status], request->resource_path, 0, NULL);
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
