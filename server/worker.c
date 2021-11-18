#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "server/server_config.h"
#include "server/worker.h"
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
    
                log_info("[%s] open connection (client %d)\n", worker_name, client_fd);

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
    
                log_info("[%s] close connection (client %d)\n", worker_name, client_fd);

                hash_map_remove(storage->opened_files, client_fd);
                /* if (storage_remove_client(storage, client_fd) != 0) {
                    printf("client %d was not removed from client table\n", client_fd);
                } */

                close(client_fd);
                client_fd = -1;
                break;
            }
            
            case OPEN_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components, check for availability and cache the file
    
                log_info("[%s] open file (%s)\n", worker_name, request->resource_path);

                int status = 0; // will be the final response status

                list_t *files_opened_by_client; 
                files_opened_by_client = (list_t*)hash_map_get(storage->opened_files, client_fd);
                if (files_opened_by_client == NULL) {
                    log_error("opened files list not recoverable\n");
                    // exit?
                }

                /* Check the flags received  */

                int flags = *(int*)request->body;

                if (CHK_FLAG(flags, O_CREATE)) { // file is O_CREATE, empty file will be created

                    log_info("[%s] create file (%s)\n", worker_name, request->resource_path);

                    if (hash_map_get(storage->files, request->resource_path) != NULL) {
                        send_response(client_fd, FILE_EXISTS, status_message[FILE_EXISTS], request->resource_path, 0, NULL);
                        break;
                    }

                    file_t *new_file = calloc(1, sizeof(file_t));
                    strcpy(new_file->path, request->resource_path);
                    new_file->size = 0;
                    new_file->fresh = 1;

                    /* Checks if a file should be expelled to make place for the new one */
                    list_t *expelled_files = list_create(NULL, free_file, NULL);
                    if (expelled_files == NULL) {
                        log_fatal("[%s] list_create failed: %s\n", worker_name, strerror(errno));
                        send_response(client_fd, INTERNAL_ERROR, status_message[INTERNAL_ERROR], 
                                        "", 0, NULL);
                        exit(EXIT_FAILURE);
                        // exit?
                    }

                    if (storage->no_of_files + 1 > storage->max_files) {

                        char *removed_file_path = (char*)list_remove_head(storage->basic_fifo);
                        file_t *removed_file = storage_remove_file(storage, removed_file_path);
                        list_insert_tail(expelled_files, removed_file); 

                        status = FILES_EXPELLED;

                        file_t *expelled = (file_t*)list_remove_head(expelled_files);
                        if (expelled == NULL) {
                            log_error("file was removed but its not recoverable\n");
                            status = INTERNAL_ERROR;
                        } else {
                            log_info("[%s] sending expelled file (%s)\n", worker_name, expelled->path);
                        }
                    }

                    /* At this point we should be able to safely add the file */

                    hash_map_insert(storage->files, new_file->path, new_file);
                    list_insert_tail(storage->basic_fifo, new_file->path);
                    storage->no_of_files++;
                    list_destroy(expelled_files);
                }


                /* Other flags yet to be implemented */

                /* Adds the file to list of files opened by client */

                char filename[MAX_NAME];
                strcpy(filename, request->resource_path);
                list_insert_tail(files_opened_by_client, filename);
                hash_map_insert(storage->opened_files, client_fd, files_opened_by_client);
    
                send_response(client_fd, status, status_message[status], "", 0, NULL);

                break;
            }
            
            case CLOSE_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components

                int status = 0; // will be the final response status

                list_t *files_opened_by_client; 
                files_opened_by_client = (list_t*)hash_map_get(storage->opened_files, client_fd);
                if (files_opened_by_client == NULL) {
                    log_error("opened files list not recoverable\n");
                    // exit?
                }

                if (list_remove_element(files_opened_by_client, request->resource_path) != 0)
                    status = NOT_FOUND;

                hash_map_insert(storage->opened_files, client_fd, files_opened_by_client);
                send_response(client_fd, status, status_message[status], request->resource_path, 0, NULL);

                break;
            }
            
            case WRITE_FILE: {
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components, check for availability and cache the file
    
                log_info("[%s] write file (%s)\n", worker_name, request->resource_path);
    
                /* Checks if the request contains any content */

                if (request->body_size == 0 || request->body == NULL) {
                    log_error("[%s] body of message is missing\n", worker_name);
                    send_response(client_fd, MISSING_BODY, status_message[MISSING_BODY], "", 0, NULL);
                    break;
                }


                /* Creates the file */
                file_t *file = storage_get_file(storage, client_fd, request->resource_path);
                if (file == NULL) {
                    send_response(client_fd, NOT_FOUND, status_message[NOT_FOUND], request->resource_path, 0, NULL);
                    break;
                }

                if (file->fresh == 0) {
                    send_response(client_fd, FILE_EXISTS, status_message[FILE_EXISTS], request->resource_path, 0, NULL);
                    break;
                }

                file->fresh = 0;
                file->size = request->body_size;
                file->contents = calloc(1, file->size);
                memcpy(file->contents, request->body, file->size);

                /* Checks if a file should be expelled to make place for the new one */
                list_t *expelled_files = list_create(NULL, free_file, NULL);
                if (expelled_files == NULL) {
                    log_fatal("[%s] list_create failed: %s\n", worker_name, strerror(errno));
                    send_response(client_fd, INTERNAL_ERROR, status_message[INTERNAL_ERROR], 
                                    "", 0, NULL);
                    exit(EXIT_FAILURE);
                    // exit?
                }

                // int no_expelled = storage_FIFO_replace(storage, 0, request->body_size, expelled_files);
                int no_expelled = 0;
                if (no_expelled) {
                    /* if (no_expelled > storage->max_files - 1) {
                            send_response(client_fd, INTERNAL_ERROR, status_message[INTERNAL_ERROR], "", 0, NULL);
                            exit(EXIT_FAILURE);
                    } */
                    while (no_expelled > 0) {

                        file_t *expelled = (file_t*)list_remove_head(expelled_files);
                        if (expelled == NULL) {
                            log_error("[%s] a file was expelled but could not be recovered\n", worker_name);
                            send_response(client_fd, INTERNAL_ERROR, status_message[INTERNAL_ERROR], "", 0, NULL);
                        } else {
                            log_info("[%s] sending expelled file (%s)\n", worker_name, expelled->path);
                            send_response(client_fd, FILES_EXPELLED, status_message[FILES_EXPELLED], expelled->path, 
                                            expelled->size, expelled->contents);
                        }
                        
                        no_expelled--;
                    }
                }

                hash_map_insert(storage->files, file->path, file);
                storage->current_size += file->size;
                list_destroy(expelled_files);
                send_response(client_fd, SUCCESS, status_message[SUCCESS], "", 0, NULL);
                break;
            }

            case WRITE_DIRECTORY: {
                break;
            }

            case READ_FILE: {
                // TODO: concurrent access to storage
                log_info("[%s] read file (%s)\n", worker_name, request->resource_path);
    
                file_t *file;
    
                file = storage_get_file(storage, client_fd, request->resource_path);
                if (file == NULL) {
                    log_error("[%s] file (%s) could not be found\n", worker_name, request->resource_path);
                    send_response(client_fd, NOT_FOUND, status_message[NOT_FOUND], request->resource_path, 0, NULL);
                } else {
                    send_response(client_fd, SUCCESS, status_message[SUCCESS], file->path, file->size, file->contents);
                }
    
                break;
            }

            case READ_N_FILES: {
                break;
            }

            case DELETE_FILE: { // NOT WORKING
                // TODO: concurrent access to storage
                // WHAT ELSE? Ensure all request components

                log_info("[%s] delete file (%s)\n", worker_name, request->resource_path);
                
                int status = 0;
                file_t *to_remove;
                if ((to_remove = storage_remove_file(storage, request->resource_path)) == NULL) {
                    switch (errno) {
                        // Will be more
                        case ENOENT: {
                            log_error("[%s] file (%s) could not be found\n", worker_name, request->resource_path);
                            status = NOT_FOUND; break; 
                        }
                    }
                } else {
                    // free_file(to_remove);
                }

                send_response(client_fd, status, status_message[status], "", 0, NULL);
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

    log_info("[%s] terminating\n", worker_name);
}
