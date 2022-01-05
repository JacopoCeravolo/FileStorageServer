#include "server/worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "server/server_config.h"

void*
worker_thread(void* args)
{
   assert(args);

   int pipe_fd = ((worker_arg_t*)args)->pipe_fd;
   int worker_id = ((worker_arg_t*)args)->worker_id;
   // concurrent_queue_t *requests = ((worker_arg_t*)args)->requests;

   free(args);

    do {

        if (shutdown_now == 1) { break; continue;}
  
        lock_return((&request_queue_mtx), -1);

        while (request_queue->length == 0) {
            cond_wait_return((&request_queue_full), (&request_queue_mtx), -1);
        }

        int *ptr = (int*)list_remove_head(request_queue);

        unlock_return(&(request_queue_mtx), -1);
        
        int client_fd = *ptr;
        free(ptr);
        
        if (client_fd == -1) {break; continue; }
        
        request_t *request;
        request = recv_request(client_fd);
        if (request == NULL) {
          log_error("request could not be received: %s\n", strerror(errno));
          close(client_fd);
          client_fd = -1;
          goto _write_pipe;
        }


        log_debug("[WORKER %d] new request\n", worker_id);

        switch (request->type) {

            case OPEN_CONNECTION: {
                int status = 0;
                // status = open_connection_handler(client_fd);
                if (shutdown_now) status = INTERNAL_ERROR;
                send_response(client_fd, status, get_status_message(status), 0, "", 0, NULL);
                log_info("(WORKER %d) [OPEN CONNECTION]: %s\n", worker_id, get_status_message(status));
                break;
            }
    
            case CLOSE_CONNECTION: {
                int status = 0;
                // status = close_connection_handler(client_fd);
                // if (status != 0) log_error("an error occurred when closing client %d connection\n", client_fd);
                // log_info("client %d connection closed\n", client_fd);
                close(client_fd);
                client_fd = -1;
                log_info("(WORKER %d) [CLOSE CONNECTION]: %s\n", worker_id, get_status_message(status));
                break;
            }
            
            case OPEN_FILE: {
                int status = open_file_handler(client_fd, request);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                log_info("(WORKER %d) [OPEN FILE]: %s\n", worker_id, get_status_message(status));
                break;
            }
            
            case CLOSE_FILE: {
                int status = close_file_handler(client_fd, request);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                log_info("(WORKER %d) [CLOSE FILE]: %s\n", worker_id, get_status_message(status));
                break;
            }
            
            case WRITE_FILE: {
                list_t *expelled_files = list_create(NULL, free_file, NULL);
                int status = write_file_handler(client_fd, request, expelled_files);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                /* if (status == FILES_EXPELLED) {
                    int how_many = list_length(expelled_files);
                    send_response(client_fd, status, get_status_message(status), "", sizeof(int), (void*)&how_many);

                    while ((how_many--) > 0) {

                        file_t *to_send = (file_t*)list_remove_head(expelled_files);

                        send_response(client_fd, status, get_status_message(status), to_send->path, to_send->size, to_send->contents);
                        log_info("[FIFO REPLACEMENT] file [%s] was expelled\n", to_send->path);
                        // free_file(to_send);

                    }
                } else {
                    send_response(client_fd, status, get_status_message(status), "", 0, NULL);
                } */
                list_destroy(expelled_files);
                log_info("(WORKER %d) [WRITE FILE]: %s\n", worker_id, get_status_message(status));
                break;
            }

            case READ_FILE: {
                void *read_buffer = NULL;
                size_t buffer_size = 0;
                int status = read_file_handler(client_fd, request, &read_buffer, &buffer_size);
                /* if (status == SUCCESS && ((buffer_size < 0) || (read_buffer == NULL))) {
                    send_response(client_fd, INTERNAL_ERROR, status_message[INTERNAL_ERROR], "", 0, NULL);
                } */
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, buffer_size, read_buffer);
                log_info("(WORKER %d) [READ FILE]: %s\n", worker_id, get_status_message(status));
                if (read_buffer) free(read_buffer);
                break;
            }

            case READ_N_FILES: {
                list_t *files_list = list_create(NULL, free_file, NULL);
                int status = read_n_files_handler(client_fd, request, files_list);
                if (status != SUCCESS) {
                    send_response(client_fd, status, get_status_message(status), 0, "", 0, NULL);
                } else {
                    int how_many = list_length(files_list);
                    send_response(client_fd, SUCCESS, get_status_message(SUCCESS), 0, "", sizeof(int), (void*)&how_many);

                    while ((how_many--) > 0) {

                        file_t *to_send = (file_t*)list_remove_head(files_list);

                        send_response(client_fd, SUCCESS, get_status_message(SUCCESS), strlen(to_send->path) + 1, to_send->path, to_send->size, to_send->contents);

                    }
                }
                log_info("(WORKER %d) [READ N FILES]: %s\n", worker_id, get_status_message(status));
                list_destroy(files_list);
                break;
            }

            case REMOVE_FILE: { 
                int status = remove_file_handler(client_fd, request);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                log_info("(WORKER %d) [REMOVE FILE]: %s\n", worker_id, get_status_message(status));
                break;
            }

            case LOCK_FILE: {
                int status = lock_file_handler(client_fd, request);
                if (status != MISSING_BODY) send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                log_info("(WORKER %d) [LOCK FILE]: %s\n", worker_id, get_status_message(status));
                break;
            }

            case UNLOCK_FILE: {
                int status = unlock_file_handler(client_fd, request);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                log_info("(WORKER %d) [UNLOCK FILE]: %s\n", worker_id, get_status_message(status));
                break;
            }
        
        } 

_write_pipe:  
        write(pipe_fd, &client_fd, sizeof(int));
        free_request(request);

    } while (shutdown_now == 0);

    return NULL;
}

/* int
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

        free(file_name);
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
 */

int
open_file_handler(int client_fd, request_t *request)
{

    log_debug("[OPEN FILE] client (%d) opening file [%s]\n", client_fd, request->file_path);

    
    int status = 0; // will be the final response status

    // Checks what flags were specified
    int flags = *(int*)request->body;
    
    if (CHK_FLAG(flags, O_CREATE)) { // flag is O_CREATE, empty file will be created
    
        log_debug("creating file [%s]\n", request->file_path);

        // Check whether file already exists

        lock_return(&(storage->access), INTERNAL_ERROR);

        if (storage_get_file(storage, request->file_path) != NULL) {
            log_error("[OPEN FILE] file [%s] already exists\n", request->file_path);
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return FILE_EXISTS;
        }

        // Creates new file

        file_t *new_file = storage_create_file(request->file_path);
        if (new_file == NULL) {
            log_error("[OPEN FILE] could not create file [%s]\n", new_file->path);
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return INTERNAL_ERROR;
        }

        // Makes space for the new file if necessary
        if (storage->no_of_files + 1 > storage->max_files) {

            char *to_remove_path = (char*)list_remove_head(storage->basic_fifo);

            if (to_remove_path == NULL) {
                unlock_return(&(storage->access), INTERNAL_ERROR);
                return INTERNAL_ERROR;
            }

            storage_remove_file(storage, to_remove_path);

            log_info("[FIFO REPLACEMENT] file expelled when adding [%s]\n", request->file_path);
        }
        /* if (storage->no_of_files + 1 > storage->max_files) {
            
            char *to_remove_path = (char*)list_remove_head(storage->basic_fifo);
            if (to_remove_path == NULL) {
                log_error("[OPEN FILE] (name null) could not make space for file [%s]\n", new_file->path);
                unlock_return(&(storage->access), INTERNAL_ERROR);
                return INTERNAL_ERROR;
            }

            file_t *to_remove = storage_get_file(storage, to_remove_path);
            if (to_remove == NULL) {
                log_error("[OPEN FILE] (file null) [%s] could not make space for file [%s]\n",to_remove_path,  new_file->path);
                unlock_return(&(storage->access), INTERNAL_ERROR);
                return INTERNAL_ERROR;
            }

            // Filename memory location will be freed after storage_remove
            char tmp_buf[256];
            strcpy(tmp_buf, to_remove_path);

            storage->current_size = storage->current_size - to_remove->size;
            storage->no_of_files--;
            hash_map_remove(storage->files, to_remove->path);

            log_info("[OPEN FILE] file [%s] expelled\n", tmp_buf);
        }
 */
        // Adds the file to storage and to list of filenames

        storage_add_file(storage, new_file);

        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_debug("file [%s] added\n", request->file_path);
    }

    if (CHK_FLAG(flags, O_LOCK)) { // flag is O_LOCK, if file exists it will be locked

        log_debug("locking file [%s]\n", request->file_path);

        // Check whether file exists

        lock_return(&(storage->access), INTERNAL_ERROR);

        file_t *file = storage_get_file(storage, request->file_path);
        if (file == NULL) {
            log_error("[OPEN FILE] file [%s] doesn't exists\n");
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return NOT_FOUND;
        }

        // If file is already locked by another client
        if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
            log_error("[OPEN FILE] file [%s] is locked by client (%d)\n", file->path,file->locked_by);
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return UNAUTHORIZED; 
        }

        // Locks file
        file->locked_by = client_fd;
        SET_FLAG(file->flags, O_LOCK);

        unlock_return(&(storage->access), INTERNAL_ERROR);
        /* LOCK_RETURN(&(file->access), INTERNAL_ERROR); */
        log_debug("file [%s] locked\n", request->file_path);
    }

    char print_flags[64] = "";
    if (CHK_FLAG(flags, O_CREATE)) strcat(print_flags, "(O_CREATE)");
    if (CHK_FLAG(flags, O_LOCK)) strcat(print_flags, "(O_LOCK)");
    
    log_debug("[OPEN FILE] file [%s] successfully opened %s by client (%d)\n", request->file_path, print_flags, client_fd);

    return status;
}

int
close_file_handler(int client_fd, request_t *request)
{
    log_debug("client %d closing file [%s]\n", client_fd, request->file_path);

    int status = 0; // will be the final response status

    lock_return(&(storage->access), INTERNAL_ERROR);

    /* Check whether file exists */
    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        log_error("[CLOSE FILE] file [%s] doesn't exists\n", request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return NOT_FOUND;
    }

    /* If file was locked by this client, performs unlock */
    if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
        
        CLR_FLAG(file->flags, O_LOCK);
        file->locked_by = -1;

        storage_update_file(storage, file);
        log_debug("client %d unlocked file [%s] before closing it\n", client_fd, request->file_path);
    }

    unlock_return(&(storage->access), INTERNAL_ERROR);

    log_debug("[CLOSE FILE] file [%s] successfully closed by client (%d)\n", request->file_path, client_fd);
    return status;
}

int
write_file_handler(int client_fd, request_t *request, list_t *expelled_files)
{
    log_debug("client %d writing file [%s]\n", client_fd, request->file_path);

    int status = 0; // will be the final response status

    /* Checks if the request contains any content */
    if (request->body_size == 0 || request->body == NULL) {
        log_error("[WRITE FILE] client (%d) bad write request\n", client_fd);
        return MISSING_BODY;
    }

    if (request->body_size > storage->max_size) {
        log_error("[WRITE FILE] file [%s] is too big\n", request->file_path);
        return BAD_REQUEST; // TOO_BIG
    }

    lock_return(&(storage->access), INTERNAL_ERROR);
    
    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        log_error("[WRITE FILE] file [%s] doesn't exists\n", request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return NOT_FOUND;
    }

    // SHOULD ALSO CHECK THE FLAG
    if ((file->locked_by != client_fd)) {
        log_error("[WRITE FILE] client (%d) must lock file [%s] before writing\n", client_fd, request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return UNAUTHORIZED;
    }

    if (!CHK_FLAG(file->flags, O_CREATE)) {
        log_error("[WRITE FILE] file [%s] exists and cannot be rewritten\n", request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return FILE_EXISTS;
    }
    

    CLR_FLAG(file->flags, O_CREATE);
    file->size = request->body_size;
    file->contents = calloc(1, file->size);
    memcpy(file->contents, request->body, file->size);

    /* Checks if a file should be expelled to make place for the new one */
    if (storage->current_size + file->size > storage->max_size) {
        log_debug("[WRITE FILE] about to expell some files\n");

        int how_many = storage_FIFO_replace(storage, 0, file->size, expelled_files);
        
        if ( how_many != list_length(expelled_files) ) {
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return INTERNAL_ERROR;
        }

        send_response(client_fd, FILES_EXPELLED, get_status_message(FILES_EXPELLED), 0, "", sizeof(int), (void*)&how_many);

        while ((how_many--) > 0) {

            file_t *to_send = (file_t*)list_remove_head(expelled_files);

            send_response(client_fd, FILES_EXPELLED, get_status_message(FILES_EXPELLED), strlen(to_send->path) + 1, to_send->path, to_send->size, to_send->contents);
            log_info("[FIFO REPLACEMENT] file [%s] was expelled\n", to_send->path);
            // free_file(to_send);
        }
    }

    storage_update_file(storage, file);
    storage->current_size += file->size;

    unlock_return(&(storage->access), INTERNAL_ERROR);

    log_debug("[WRITE FILE] file [%s] successfully written by client (%d)\n", request->file_path, client_fd);
    return SUCCESS;
            
}

int
read_file_handler(int client_fd, request_t *request, void** read_buffer, size_t *size)
{
    log_debug("client %d reading file [%s]\n", client_fd, request->file_path);

    int status = 0; // will be the final response status

    lock_return(&(storage->access), INTERNAL_ERROR);

    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        log_error("[READ FILE] file [%s] doesn't exists\n", request->file_path);
        return NOT_FOUND;
    }

    *read_buffer = calloc(1, file->size);
    *size = file->size;
    memcpy(*read_buffer, file->contents, file->size);

    unlock_return(&(storage->access), INTERNAL_ERROR);

    log_debug("[READ FILE] file [%s] successfully red by client (%d)\n", request->file_path, client_fd);
    return status;
}

int
read_n_files_handler(int client_fd, request_t *request, list_t *files_list)
{
    int how_many = *(int*)request->body;
    if ((how_many <= 0) || (how_many > storage->max_size)) {
        how_many = storage->max_size;
    }

    lock_return(&(storage->access), INTERNAL_ERROR);

    node_t *iter = storage->basic_fifo->head;

    for (int i = 0; i < how_many && iter != NULL; i++) {

        // should be random, for now in fifo order
        char *path = (char*)iter->data;
        if (path == NULL) { unlock_return(&(storage->access), INTERNAL_ERROR); return INTERNAL_ERROR; }
        file_t *file = storage_get_file(storage, path);
        if (file == NULL) { unlock_return(&(storage->access), INTERNAL_ERROR); return INTERNAL_ERROR; }

        list_insert_tail(files_list, file);
        iter = iter->next;
    }

    unlock_return(&(storage->access), INTERNAL_ERROR);
    return SUCCESS;
}

int
remove_file_handler(int client_fd, request_t *request)
{

    log_debug("client %d removing file [%s]\n", client_fd, request->file_path);

    int status = 0; // will be the final response status

    lock_return(&(storage->access), INTERNAL_ERROR);

    file_t *to_remove = storage_get_file(storage, request->file_path);
    if (to_remove == NULL) {
        log_error("[REMOVE FILE] file [%s] doesn't exists\n", request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return NOT_FOUND;
    }

    if ( (!CHK_FLAG(to_remove->flags, O_LOCK)) ||
            (CHK_FLAG(to_remove->flags, O_LOCK) && to_remove->locked_by != client_fd)) {
        log_error("[REMOVE FILE] client (%d) must lock file [%s] before removing it\n", client_fd, request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return UNAUTHORIZED;
    }

    to_remove = storage_remove_file(storage, request->file_path);

    unlock_return(&(storage->access), INTERNAL_ERROR);

    log_debug("[REMOVE FILE] file [%s] successfully removed by client (%d)\n", request->file_path, client_fd);

    return status;
}

int
lock_file_handler(int client_fd, request_t *request)
{
    log_debug("client %d locking file [%s]\n", client_fd, request->file_path);

    int status = 0; // will be the final response status

    lock_return(&(storage->access), INTERNAL_ERROR);

    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        log_error("[LOCK FILE] file [%s] doesn't exists\n", request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return NOT_FOUND;
    }

    if (!CHK_FLAG(file->flags, O_LOCK)) { // File isn't locked

        SET_FLAG(file->flags, O_LOCK);
        file->locked_by = client_fd;
        storage_update_file(storage, file);

        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_debug("[LOCK FILE] file [%s] successfully locked by client (%d)\n", request->file_path, client_fd);
        return SUCCESS;

    } else {

        if (file->locked_by == client_fd) {

            log_debug("[LOCK FILE] file [%s] successfully locked by client (%d)\n", request->file_path, client_fd);
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return SUCCESS;
        }

        list_insert_tail(file->waiting_on_lock, client_fd);

        storage_update_file(storage, file);

        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_debug("client %d waiting on lock for file [%s]\n", client_fd, request->file_path);

        return MISSING_BODY; // FOR ME TO REMEMBER THAT IT HAS TO WAIT UNTIL LOCK IS RELEASED
    }

    return status;

}

int
unlock_file_handler(int client_fd, request_t *request)
{
    log_debug("client %d unlocking file [%s]\n", client_fd, request->file_path);

    int status = 0; // will be the final response status

    lock_return(&(storage->access), INTERNAL_ERROR);

    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        log_error("[UNLOCK FILE] file [%s] doesn't exists\n", request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return NOT_FOUND;
    }

    if (!CHK_FLAG(file->flags, O_LOCK)) { // File isn't locked
        log_error("[UNLOCK FILE] file [%s] is not locked\n", request->file_path);
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return BAD_REQUEST;

    } else {

        if (file->locked_by == client_fd) {

            CLR_FLAG(file->flags, O_LOCK);
            file->locked_by = -1;
            
            storage_update_file(storage, file);

            log_debug("[UNLOCK FILE] file [%s] successfully unlocked by client (%d)\n", request->file_path, client_fd);
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return SUCCESS;
        
        } else {
            log_error("[UNLOCK FILE] file [%s] is locked by client (%d)\n", request->file_path, file->locked_by);
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return BAD_REQUEST;
        }
    }

    return status;
}