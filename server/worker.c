#include "server/worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "server/server_config.h"

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

int
open_connection_handler(int client_fd)
{
    log_debug("[OPEN CONNECTION] new client (%d)\n", client_fd);


    // New connection, will initialize string list to keep track of files opened by client
    list_t *files_opened_by_client; 
    files_opened_by_client = list_create(string_compare, free_string, string_print);
    if (files_opened_by_client == NULL) {
        log_fatal("could not create opened file list of client (%d)\n", client_fd);
        return INTERNAL_ERROR;
    }

    // Adds the freshly created list to the hashtable of connected clients
    if (hash_map_insert(connected_clients, client_fd, files_opened_by_client) != 0) {
        
        // Insert failed returning INTERNAL_ERROR
        log_fatal("could not add opened file list of client (%d)\n", client_fd);
        return INTERNAL_ERROR;

    }

    // Successfully added new client
    log_info("[OPEN CONNECTION] successfully opened new connection with client (%d)\n", client_fd);

    return SUCCESS;
}

int
close_connection_handler(int client_fd)
{
    log_debug("[CLOSE CONNECTION] closing client (%d) connection\n", client_fd);


    // Closing client connection, retrieving list of opened files to close all of them 
    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(connected_clients, client_fd);
    if (files_opened_by_client == NULL) {

        // Failed to retreive
        log_fatal("client (%d) list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;

    }

    // List retreived, closing all files present
    while (!list_is_empty(files_opened_by_client)) {

        char *file_name = (char*)list_remove_head(files_opened_by_client);
        file_t *file = (file_t*)hash_map_get(storage->files, file_name);
        
        if (file == NULL) {
            // File might have been removed by someone else, shouldn't fail
            log_debug("couldn't find file [%s] when closing client (%d) connection\n", file_name, client_fd);
        } 
    
        else {
            // If file was locked by client, unlock it
            if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
                
                CLR_FLAG(file->flags, O_LOCK);
                file->locked_by = 0;
                hash_map_insert(storage->files, file->path, file);
                log_debug("client (%d) unlocked file [%s] before closing it\n", client_fd, file_name);
            }
        }
    }

    // Removing client from the hashtable of connected clients
    if (hash_map_remove(connected_clients, client_fd) == -1) { 
        log_error("Could not remove client form hashtable\n"); 
        // not failing, connection will still be closed
    }

    
    // Successfully closed all files
    log_info("[CLOSE CONNECTION] successfully closed connection with client (%d)\n", client_fd);

    return SUCCESS;    
}

int
open_file_handler(int client_fd, request_t *request, list_t *expelled_files)
{

    log_debug("[OPEN FILE] client (%d) opening file [%s]\n", client_fd, request->resource_path);

    
    int status = 0; // will be the final response status

    // Checks whether file exists
    /* file_t *file;
    if ((file = (file_t*)hash_map_get(storage->files, request->resource_path)) == NULL) {
        return NOT_FOUND;
    } */

    // Checks what flags were specified
    int flags = *(int*)request->body;

    
    if (CHK_FLAG(flags, O_CREATE)) { // flag is O_CREATE, empty file will be created
    
        log_debug("creating file [%s]\n", request->resource_path);

        // Check whether file already exists
        if (hash_map_get(storage->files, request->resource_path) != NULL) {
            log_error("[OPEN FILE] file [%s] already exists\n");
            return FILE_EXISTS;
        }

        // Creates new file
        file_t *new_file = calloc(1, sizeof(file_t));
        strcpy(new_file->path, request->resource_path);
        new_file->size = 0;
        new_file->locked_by = -1;
        SET_FLAG(new_file->flags, O_CREATE);
        new_file->waiting_on_lock = list_create(int_compare, NULL, print_int);
        if ((pthread_mutex_init(&(new_file->access), NULL)) != 0 ||
            (pthread_cond_init(&(new_file->available), NULL) != 0)) {
            log_error("[OPEN FILE] could not create file [%s]\n", new_file->path);
            return INTERNAL_ERROR;
        }

        // Makes space for the new file if necessary
        if (storage->no_of_files + 1 > storage->max_files) {
            
            char *removed_file_path = (char*)list_remove_head(storage->basic_fifo);
            file_t *removed_file = storage_remove_file(storage, removed_file_path);
            list_insert_tail(expelled_files, removed_file);
            log_debug("file [%s] was expelled when adding file [%s]\n", removed_file_path, request->resource_path);
            status = FILES_EXPELLED;
        }

        // Adds the file to storage and to list of filenames
        hash_map_insert(storage->files, new_file->path, new_file);
        list_insert_tail(storage->basic_fifo, new_file->path);
        storage->no_of_files++;

        log_debug("file [%s] added\n", request->resource_path);
    }

    if (CHK_FLAG(flags, O_LOCK)) { // flag is O_LOCK, if file exists it will be locked

        log_debug("locking file [%s]\n", request->resource_path);

        // Check whether file exists
        file_t *file;
        if ((file = (file_t*)hash_map_get(storage->files, request->resource_path)) == NULL) {
            return NOT_FOUND;
        }


        // If file is already locked by another client
        if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
            log_error("[OPEN FILE] file [%s] is locked by client (%d)\n", file->path,file->locked_by);
            return UNAUTHORIZED; 
        }

        // Locks file
        file->locked_by = client_fd;
        SET_FLAG(file->flags, O_LOCK);
        LOCK_RETURN(&(file->access), INTERNAL_ERROR);
        hash_map_insert(storage->files, file->path, file);

        log_debug("file [%s] locked\n", request->resource_path);
    }

    // Adds the file to the list of files opened by client
    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(connected_clients, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client (%d) list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }
    char *filename = calloc(1, MAX_NAME);
    strcpy(filename, request->resource_path);
    list_insert_tail(files_opened_by_client, filename);
    hash_map_insert(connected_clients, client_fd, files_opened_by_client);


    char print_flags[64] = "";
    if (CHK_FLAG(flags, O_CREATE)) strcat(print_flags, "(O_CREATE)");
    if (CHK_FLAG(flags, O_LOCK)) strcat(print_flags, "(O_LOCK)");
    
    log_info("[OPEN FILE] file [%s] successfully opened %s by client (%d)\n", request->resource_path, print_flags, client_fd);

    return status;
}

int
close_file_handler(int client_fd, request_t *request)
{
    log_debug("client %d closing file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    file_t *file = (file_t*)hash_map_get(storage->files, request->resource_path);
    if (file == NULL) {
        log_error("[CLOSE FILE] file [%s] doesn't exists\n", request->resource_path);
        return NOT_FOUND;
    }

    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(connected_clients, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client %d list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    if (list_find(files_opened_by_client, request->resource_path) == -1) {
        log_error("client %d must open file [%s] before closing it\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
        CLR_FLAG(file->flags, O_LOCK);
        file->locked_by = 0;
        hash_map_insert(storage->files, file->path, file);
        log_debug("client %d unlocked file [%s] before closing it\n", client_fd, request->resource_path);
    }

    list_remove_element(files_opened_by_client, request->resource_path);
    hash_map_insert(connected_clients, client_fd, files_opened_by_client);

    log_info("[CLOSE FILE] file [%s] successfully closed by client (%d)\n", request->resource_path, client_fd);
    return status;
}

int
write_file_handler(int client_fd, request_t *request, list_t *expelled_files)
{
    log_debug("client %d writing file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    /* Checks if the request contains any content */
    if (request->body_size == 0 || request->body == NULL) {
        log_error("[WRITE FILE] client (%d) bad write request\n", client_fd);
        return MISSING_BODY;
    }

    /* Creates the file */
    file_t *file = storage_get_file(storage, client_fd, request->resource_path);
    
    if (file == NULL) {
        log_error("[WRITE FILE] file [%s] doesn't exists\n", request->resource_path);
        return NOT_FOUND;
    }

    // SHOULD ALSO CHECK THE FLAG
    if (file->locked_by != client_fd) {
        log_error("[WRITE FILE] client (%d) must lock file [%s] before writing\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    if (!CHK_FLAG(file->flags, O_CREATE)) {
        log_error("[WRITE FILE] file [%s] exists and cannot be rewritten\n", request->resource_path);
        return FILE_EXISTS;
    }

    CLR_FLAG(file->flags, O_CREATE);
    file->size = request->body_size;
    file->contents = calloc(1, file->size);
    memcpy(file->contents, request->body, file->size);

    /* Checks if a file should be expelled to make place for the new one */
    /* Should use replacement algorithm */

    hash_map_insert(storage->files, file->path, file);
    storage->current_size += file->size;

    log_info("[WRITE FILE] file [%s] successfully written by client (%d)\n", request->resource_path, client_fd);
    return status;
            
}

int
read_file_handler(int client_fd, request_t *request, void** read_buffer, size_t *size)
{
    log_debug("client %d reading file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    file_t *file = (file_t*)hash_map_get(storage->files, request->resource_path);
    if (file == NULL) {
        log_error("[READ FILE] file [%s] doesn't exists\n", request->resource_path);
        return NOT_FOUND;
    }

    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(connected_clients, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client %d list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    if (list_find(files_opened_by_client, request->resource_path) == -1) {
        log_error("[READ FILE] client %d must open file [%s] before reading it\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    *read_buffer = calloc(1, file->size);
    *size = file->size;
    memcpy(*read_buffer, file->contents, file->size);

    log_info("[READ FILE] file [%s] successfully red by client (%d)\n", request->resource_path, client_fd);
    return status;
}

int
remove_file_handler(int client_fd, request_t *request)
{

    log_debug("client %d removing file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    file_t *to_remove = (file_t*)hash_map_get(storage->files, request->resource_path);
    if (to_remove == NULL) {
        log_error("[REMOVE FILE] file [%s] doesn't exists\n", request->resource_path);
        return NOT_FOUND;
    }

    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(connected_clients, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client %d list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    if (list_find(files_opened_by_client, request->resource_path) == -1) {
        log_error("[REMOVE FILE] client (%d) must open file [%s] before removing it\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    if ( (!CHK_FLAG(to_remove->flags, O_LOCK)) ||
            (CHK_FLAG(to_remove->flags, O_LOCK) && to_remove->locked_by != client_fd)) {
        log_error("[REMOVE FILE] client (%d) must lock file [%s] before removing it\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    storage->no_of_files--;
    storage->current_size = storage->current_size - to_remove->size;
    list_remove_element(storage->basic_fifo, to_remove->path);
    hash_map_remove(storage->files, to_remove->path);

    log_info("[REMOVE FILE] file [%s] successfully removed by client (%d)\n", request->resource_path, client_fd);

    return status;
}

int
lock_file_handler(int client_fd, request_t *request)
{
    log_debug("client %d locking file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    file_t *file = (file_t*)hash_map_get(storage->files, request->resource_path);
    if (file == NULL) {
        log_error("[LOCK FILE] file [%s] doesn't exists\n", request->resource_path);
        return NOT_FOUND;
    }

    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(connected_clients, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client %d list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    if (list_find(files_opened_by_client, request->resource_path) == -1) {
        log_error("[LOCK FILE] client (%d) must open file [%s] before locking it\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    if (!CHK_FLAG(file->flags, O_LOCK)) { // File isn't locked

        SET_FLAG(file->flags, O_LOCK);
        file->locked_by = client_fd;
        hash_map_insert(storage->files, file->path, file);

        log_info("[LOCK FILE] file [%s] successfully locked by client (%d)\n", request->resource_path, client_fd);
        return SUCCESS;

    } else {

        if (file->locked_by == client_fd) {

            log_info("[LOCK FILE] file [%s] successfully locked by client (%d)\n", request->resource_path, client_fd);
            return SUCCESS;
        }

        list_insert_tail(file->waiting_on_lock, client_fd);
        hash_map_insert(storage->files, file->path, file);
        log_info("client %d waiting on lock of file [%s]\n", client_fd, request->resource_path);

        return MISSING_BODY; // FOR ME TO REMEMBER THAT IT HAS TO WAIT UNTIL LOCK IS RELEASED
    }

    return status;

}

int
unlock_file_handler(int client_fd, request_t *request)
{
    log_debug("client %d unlocking file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    file_t *file = (file_t*)hash_map_get(storage->files, request->resource_path);
    if (file == NULL) {
        log_error("[UNLOCK FILE] file [%s] doesn't exists\n", request->resource_path);
        return NOT_FOUND;
    }

    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(connected_clients, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client %d list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    if (list_find(files_opened_by_client, request->resource_path) == -1) {
        log_error("[UNLOCK FILE] client (%d) must open file [%s] before unlocking it\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    if (!CHK_FLAG(file->flags, O_LOCK)) { // File isn't locked
        log_error("[UNLOCK FILE] file [%s] is not locked\n", request->resource_path);
        return BAD_REQUEST;

    } else {

        if (file->locked_by == client_fd) {
            CLR_FLAG(file->flags, O_LOCK);
            file->locked_by = 0;
            hash_map_insert(storage->files, file->path, file);

            log_info("[UNLOCK FILE] file [%s] successfully unlocked by client (%d)\n", request->resource_path, client_fd);
            return SUCCESS;
        } else {
            log_error("[UNLOCK FILE] file [%s] is locked by client (%d)\n", request->resource_path, file->locked_by);
            return BAD_REQUEST;
        }
    }

    return status;
}