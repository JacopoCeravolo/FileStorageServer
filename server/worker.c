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
    // Copying worker thread arguments
    assert(args);

    int pipe_fd = ((worker_arg_t*)args)->pipe_fd;
    int worker_id = ((worker_arg_t*)args)->worker_id;

    free(args);

    // Main worker loop
    do {

        if ( shutdown_now == 1 ) break;   // Server received shutdown signal
  
        // Reading client file descriptor from server
        lock_return((&request_queue_mtx), NULL);

        while (request_queue->length == 0) {
            cond_wait_return((&request_queue_notempty), (&request_queue_mtx), NULL);
        }

        int *tmp_ptr = (int*)list_remove_head(request_queue);

        unlock_return(&(request_queue_mtx), NULL);
        
        int client_fd = *tmp_ptr;
        free(tmp_ptr);
        
        if ( client_fd == -1 ) break;     // Server signal to worker for termination
        
        // Receiving request from client
        request_t *request = recv_request(client_fd);
        if (request == NULL) {
            log_error("Request could not be received: %s\n", strerror(errno));
            close(client_fd);
            client_fd = -1;
            write(pipe_fd, &client_fd, sizeof(int));
            free_request(request);
        }

        switch (request->type) {

            case OPEN_CONNECTION: {     
                int status = 0;
                if ( shutdown_now ) status = INTERNAL_ERROR;  // shouldn't happen
                if ( accept_connection == 0 ) status = NO_MORE_CON;
                send_response(client_fd, status, get_status_message(status), 0, "", 0, NULL);

                log_info("(WORKER %d) [  %s  ]  %-21s\n", worker_id, "openConn",  get_status_message(status));
                break;
            }
    
            case CLOSE_CONNECTION: {     
                int status = 0;
                close(client_fd);
                client_fd = -1;
                
                // Updating server status
                lock_return((&server_status_mtx), NULL); 
                server_status->current_connections--;
                unlock_return((&server_status_mtx), NULL);

                log_info("(WORKER %d) [ %s  ]  %-21s\n", worker_id, "closeConn", get_status_message(status));
                
                break;
            }
            
            case OPEN_FILE: {    
                int status = open_file_handler(worker_id, client_fd, request);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                break;
            }
            
            case CLOSE_FILE: {
                int status = close_file_handler(worker_id, client_fd, request);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                break;

            }
            
            case WRITE_FILE: {
                list_t *expelled_files = list_create(NULL, free_file, NULL);
                int status = write_file_handler(worker_id, client_fd, request, expelled_files);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                list_destroy(expelled_files);
                break;
            }

            case APPEND_TO_FILE: {
                list_t *expelled_files = list_create(NULL, free_file, NULL);
                int status = append_to_file_handler(worker_id, client_fd, request, expelled_files);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                list_destroy(expelled_files);
            }

            case READ_FILE: {
                void *read_buffer = NULL;
                size_t buffer_size = 0;
                int status = read_file_handler(worker_id, client_fd, request, &read_buffer, &buffer_size);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, buffer_size, read_buffer);
                if (read_buffer) free(read_buffer);
                break;
            }

            case READ_N_FILES: {
                list_t *files_list = list_create(NULL, free_file, NULL);
                int status = read_n_files_handler(worker_id, client_fd, request, files_list);
                send_response(client_fd, status, get_status_message(status), 0, "", 0, NULL);
                list_destroy(files_list);
                break;
            }

            case REMOVE_FILE: { 
                int status = remove_file_handler(worker_id, client_fd, request);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                break;
            }

            case LOCK_FILE: {
                int status = lock_file_handler(worker_id, client_fd, request);
                if (status != AWAITING) {
                    send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                }
                break;
            }

            case UNLOCK_FILE: {
                int status = unlock_file_handler(worker_id, client_fd, request);
                send_response(client_fd, status, get_status_message(status), request->path_len, request->file_path, 0, NULL);
                break;
            }
        } 

        if ( write(pipe_fd, &client_fd, sizeof(int)) == -1 ) {
            log_fatal("(WORKER %d) write on pipe failed: %s\n", strerror(errno));
            return NULL;
        }

        if ( request ) free_request(request);

    } while (shutdown_now == 0);

    return NULL;
}

int
open_file_handler(int worker_no, int client_fd, request_t *request)
{

    log_debug("opening file [%s]\n", request->file_path);

    int status = 0; // will be the final response status

    // Checking what flags were specified
    int flags = *(int*)request->body;
    
    if (CHK_FLAG(flags, O_CREATE)) { // flag is O_CREATE, empty file will be created
    
        log_debug("creating file [%s]\n", request->file_path);

        // Checking whether file already exists
        lock_return(&(storage->access), INTERNAL_ERROR);

        if (storage_get_file(storage, request->file_path) != NULL) {
            // File exists, log and return status
            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                worker_no, "openFile", get_status_message(FILE_EXISTS), request->file_path);

            return FILE_EXISTS;
        }

        // Creating the file
        file_t *new_file = storage_create_file(request->file_path);
        if (new_file == NULL) {
            // Fatal error
            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                worker_no, "openFile", get_status_message(INTERNAL_ERROR), request->file_path);

            return INTERNAL_ERROR;
        }

        // Makes space for the new file if necessary
        if (storage->no_of_files + 1 > storage->max_files) {

            char *to_remove_path = (char*)list_remove_head(storage->fifo_queue);
            if (to_remove_path == NULL) {
                // Fatal error
                unlock_return(&(storage->access), INTERNAL_ERROR);

                log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                    worker_no, "openFile", get_status_message(INTERNAL_ERROR), request->file_path);

                return INTERNAL_ERROR;
            }

            
            // Removing file
            log_info("(WORKER %d) [%s]  %-21s : %s\n", 
                worker_no, "FIFO replace", get_status_message(FILES_EXPELLED), to_remove_path);
            storage_remove_file(storage, to_remove_path);

        }
        
        
        // Adding empty file to storage
        storage_add_file(storage, new_file);

        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_debug("file [%s] added\n", request->file_path);
    }

    if (CHK_FLAG(flags, O_LOCK)) { // flag is O_LOCK, if file exists it will be locked

        log_debug("locking file [%s]\n", request->file_path);

        // Checking whether file already exists
        lock_return(&(storage->access), INTERNAL_ERROR);

        file_t *file = storage_get_file(storage, request->file_path);
        if (file == NULL) {
            // File doesn't exists, log and return
            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                worker_no, "openFile", get_status_message(NOT_FOUND), request->file_path);

            return NOT_FOUND;
        }

        // Checking whether file is already locked
        if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
            
            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                worker_no, "openFile", get_status_message(UNAUTHORIZED), request->file_path);

            return UNAUTHORIZED; 
        }

        // Locking file file
        file->locked_by = client_fd;
        SET_FLAG(file->flags, O_LOCK);

        storage_update_file(storage, file);

        unlock_return(&(storage->access), INTERNAL_ERROR);
 
        log_debug("file [%s] locked\n", request->file_path);
    }

    if (CHK_FLAG(flags, O_NOFLAG)) { // flag is O_NOFLAG, checking whether file exists
        
        // Checking whether file already exists
        lock_return(&(storage->access), INTERNAL_ERROR);

        file_t *file = storage_get_file(storage, request->file_path);
        if (file == NULL) {
            // File doesn't exists, log and return
            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                worker_no, "openFile", get_status_message(NOT_FOUND), request->file_path);

            return NOT_FOUND;
        }

        unlock_return(&(storage->access), INTERNAL_ERROR);
    }

    // Printing found flags
    char print_flags[64] = "";
    if (CHK_FLAG(flags, O_CREATE)) strcat(print_flags, "(O_CREATE)");
    if (CHK_FLAG(flags, O_LOCK)) strcat(print_flags, "(O_LOCK)");
    if (CHK_FLAG(flags, O_NOFLAG)) strcat(print_flags, "(O_NOFLAG)");
    
    
    log_info("(WORKER %d) [  %s  ]  %-21s : %s : %s\n", 
        worker_no, "openFile", get_status_message(status), request->file_path, print_flags);

    return status;
}

int
close_file_handler(int worker_no, int client_fd, request_t *request)
{
    log_debug("closing file [%s]\n", request->file_path);

    int status = 0; // will be the final response status

    lock_return(&(storage->access), INTERNAL_ERROR);

    // Checking whether file exists
    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        // File doesn't exists, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [ %s  ]  %-21s : %s\n", 
            worker_no, "closeFile", get_status_message(NOT_FOUND), request->file_path);
        
        return NOT_FOUND;
    }

    // Checking whether file was locked by this client
    if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
        
        // Unlocking file
        CLR_FLAG(file->flags, O_LOCK);
        file->locked_by = -1;

        storage_update_file(storage, file);
        
        log_debug("unlocked file [%s] before closing it\n", request->file_path);
    }

    unlock_return(&(storage->access), INTERNAL_ERROR);


    log_info("(WORKER %d) [ %s  ]  %-21s : %s\n", 
        worker_no, "closeFile", get_status_message(status), request->file_path);

    return status;
}

int
write_file_handler(int worker_no, int client_fd, request_t *request, list_t *expelled_files)
{
    log_debug("writing file [%s]\n", request->file_path);

    int status = 0; // will be the final response status

    // Checking if request contains any content
    if (request->body_size == 0 || request->body == NULL) {
        
        log_info("(WORKER %d) [ %s  ]  %-21s : %s : %lu bytes\n", 
            worker_no, "writeFile", get_status_message(BAD_REQUEST), request->file_path, request->body_size);

        return BAD_REQUEST;
    }


    // Checking whether file exists
    lock_return(&(storage->access), INTERNAL_ERROR);
    
    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        // File doesn't exists, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [ %s  ]  %-21s : %s : %lu bytes\n", 
            worker_no, "writeFile", get_status_message(NOT_FOUND), request->file_path, request->body_size);

        return NOT_FOUND;
    }

    // Checking whether file is locked by this client
    if ( (file->locked_by != client_fd) ) {
        // Client doesn't have permission, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [ %s  ]  %-21s : %s : %lu bytes\n", 
            worker_no, "writeFile", get_status_message(UNAUTHORIZED), request->file_path, request->body_size);

        return UNAUTHORIZED;
    }

    // Checking if file is too big
    if (request->body_size > storage->max_size) {
        // File is too big, removing empty file previuosly created, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);
        storage_remove_file(storage, request->file_path);
        
        log_info("(WORKER %d) [ %s  ]  %-21s : %s : %lu bytes\n", 
            worker_no, "writeFile", get_status_message(FILE_TOO_BIG), request->file_path, request->body_size);
        
        return FILE_TOO_BIG; 
    }

    // Checking if file was freshly created, otherwise it cannot be overwritten
    if (!CHK_FLAG(file->flags, O_CREATE)) {
       
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [ %s  ]  %-21s : %s : %lu bytes\n", 
            worker_no, "writeFile", get_status_message(FILE_EXISTS), request->file_path, request->body_size);

        return FILE_EXISTS;
    }

    // Checking if some files show be expelled to make space 
    if (storage->current_size + request->body_size > storage->max_size) {
        
        log_debug("about to expell some files\n");

        int how_many = storage_FIFO_replace(storage, 0, request->body_size, expelled_files);
        
        if ( how_many != list_length(expelled_files) ) { // shouldn't happen
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return INTERNAL_ERROR;
        }

        // Sending response to client with number of files expelled
        if ( send_response(client_fd, FILES_EXPELLED, get_status_message(FILES_EXPELLED), 0, "", sizeof(int), (void*)&how_many) != 0 ) {
            unlock_return(&(storage->access), INTERNAL_ERROR);
            return INTERNAL_ERROR;
        }

        // Starts sending expelled files to client
        while ((how_many--) > 0) {

            file_t *to_send = (file_t*)list_remove_head(expelled_files);

            // Sending current expelled file to client
            send_response(client_fd, FILES_EXPELLED, get_status_message(FILES_EXPELLED), 
                strlen(to_send->path) + 1, to_send->path, to_send->size, to_send->contents);

           
            log_info("(WORKER %d) [%s]  %-21s : %s\n", 
                worker_no, "FIFO replace", get_status_message(FILES_EXPELLED), to_send->path);
        }
    }

    // Updating file contents
    CLR_FLAG(file->flags, O_CREATE);
    file->size = request->body_size;
    file->contents = calloc(1, file->size);
    memcpy(file->contents, request->body, file->size);
    storage_update_file(storage, file);
    list_insert_tail(storage->fifo_queue, file->path);
    storage->current_size += file->size;

    unlock_return(&(storage->access), INTERNAL_ERROR);

    log_info("(WORKER %d) [ %s  ]  %-21s : %s : %lu bytes\n", 
        worker_no, "writeFile", get_status_message(status), request->file_path, request->body_size);

    return status;
            
}

int
append_to_file_handler(int worker_no, int client_fd, request_t *request, list_t *expelled_files)
{
    log_debug("appending to file [%s]\n", request->file_path);

    int status = 0; // will be the final response status

    // Checking whether request contains any content
    if (request->body_size == 0 || request->body == NULL) {
        
        log_info("(WORKER %d) [%s]  %-21s : %s : %lu bytes\n", 
            worker_no, "appendToFile", get_status_message(BAD_REQUEST), request->file_path, request->body_size);

        return BAD_REQUEST;
    }

    // Checking if file is too big for storage
    if (request->body_size > storage->max_size) {
        
        log_info("(WORKER %d) [%s]  %-21s : %s : %lu bytes\n", 
            worker_no, "appendToFile", get_status_message(FILE_TOO_BIG), request->file_path, request->body_size);
        
        return FILE_TOO_BIG; 
    }

    // Checking whether file exists
    lock_return(&(storage->access), INTERNAL_ERROR);
    
    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        // File doesn't exists, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [%s]  %-21s : %s : %lu bytes\n", 
            worker_no, "appendToFile", get_status_message(NOT_FOUND), request->file_path, request->body_size);

        return NOT_FOUND;
    }

    // Checking whether client has locked the file
    if ( (file->locked_by != client_fd) ) {
        // Client doesn't have permission to append, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);
        
        log_info("(WORKER %d) [%s]  %-21s : %s : %lu bytes\n", 
            worker_no, "appendToFile", get_status_message(UNAUTHORIZED), request->file_path, request->body_size);
        
        return UNAUTHORIZED;
    }

    // check for fifo replacement



    // Updating file contents
    size_t new_size = file->size + request->body_size;
    file->contents = realloc(file->contents, new_size);
    if (file->contents == NULL) {
        errno = ENOMEM;
        unlock_return(&(storage->access), INTERNAL_ERROR);
        return INTERNAL_ERROR;
    }

    memcpy((file->contents + file->size), request->body, request->body_size);
    file->size = new_size;
    storage_update_file(storage, file);
    storage->current_size += request->body_size;

    unlock_return(&(storage->access), INTERNAL_ERROR);


    log_info("(WORKER %d) [%s]  %-21s : %s : %lu bytes\n", 
        worker_no, "appendToFile", get_status_message(status), request->file_path, request->body_size);

    return status;

}

int
read_file_handler(int worker_no, int client_fd, request_t *request, void** read_buffer, size_t *size)
{
    log_debug("reading file [%s]\n", request->file_path);

    int status = 0; // will be the final response status

    // Checking whether file exists
    lock_return(&(storage->access), INTERNAL_ERROR);

    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        // File doesn't exists, log and return
        log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                            worker_no, "readFile", get_status_message(NOT_FOUND), request->file_path);
        
        return NOT_FOUND;
    }

    // Copying file contents into reading buffer
    *read_buffer = calloc(1, file->size);
    if ( *read_buffer == NULL ) {
        errno = ENOMEM;
        return INTERNAL_ERROR;
    }

    *size = file->size;
    memcpy(*read_buffer, file->contents, file->size);

    unlock_return(&(storage->access), INTERNAL_ERROR);

    
    log_info("(WORKER %d) [  %s  ]  %-21s : %s : %lu bytes\n", 
        worker_no, "readFile", get_status_message(status), request->file_path, *size);
    
    return status;
}

int
read_n_files_handler(int worker_no, int client_fd, request_t *request, list_t *files_list)
{
    // Checking how many files to read
    int how_many = *(int*)request->body;
    if ((how_many <= 0) || (how_many > storage->max_size)) {
        how_many = storage->max_size;
    }

    lock_return(&(storage->access), INTERNAL_ERROR);

    node_t *iter = storage->fifo_queue->head;

    for (int i = 0; i < how_many && iter != NULL; i++) {

        char *path = (char*)iter->data;
        if (path == NULL) { 
            unlock_return(&(storage->access), INTERNAL_ERROR); 

            log_info("(WORKER %d) [ %s  ]  %-21s : %s : %lu bytes\n", 
                worker_no, "readNFiles", get_status_message(INTERNAL_ERROR), "", 0);

            return INTERNAL_ERROR; 
        }

        file_t *file = storage_get_file(storage, path);
        if (file == NULL) { 
            unlock_return(&(storage->access), INTERNAL_ERROR); 

            log_info("(WORKER %d) [ %s  ]  %-21s : %s : %lu bytes\n", 
                worker_no, "readNFiles", get_status_message(INTERNAL_ERROR), "", 0);

            return INTERNAL_ERROR; 
        }

        list_insert_tail(files_list, file);
        iter = iter->next;
    }

    unlock_return(&(storage->access), INTERNAL_ERROR);

    how_many = list_length(files_list);
        
    // Sending number of files to client
    send_response(client_fd, SUCCESS, get_status_message(SUCCESS), 0, "", sizeof(int), (void*)&how_many);

    // Start sending files to client
    while ( (how_many--) > 0) {

        file_t *to_send = (file_t*)list_remove_head(files_list);

        // Sending current file to client
        send_response(client_fd, SUCCESS, get_status_message(SUCCESS), 
            strlen(to_send->path) + 1, to_send->path, to_send->size, to_send->contents);

    }

    log_info("(WORKER %d) [ %s ]  %-21s\n", 
        worker_no, "readNFiles", get_status_message(SUCCESS));

    return SUCCESS;
}

int
remove_file_handler(int worker_no, int client_fd, request_t *request)
{

    log_debug("removing file [%s]\n", request->file_path);

    int status = 0; // will be the final response status

    // Checking whether file exists
    lock_return(&(storage->access), INTERNAL_ERROR);

    file_t *to_remove = storage_get_file(storage, request->file_path);
    if (to_remove == NULL) {
        // File doesn't exists, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [ %s ]  %-21s : %s\n", 
            worker_no, "removeFile", get_status_message(NOT_FOUND), request->file_path);

        return NOT_FOUND;
    }

    // Checking whether client has permission to remove file
    if ( (!CHK_FLAG(to_remove->flags, O_LOCK)) ||
            (CHK_FLAG(to_remove->flags, O_LOCK) && to_remove->locked_by != client_fd)) {
        
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [ %s ]  %-21s : %s\n", 
            worker_no, "removeFile", get_status_message(UNAUTHORIZED), request->file_path);
        
        return UNAUTHORIZED;
    }

    // Removing file
    storage_remove_file(storage, request->file_path);

    unlock_return(&(storage->access), INTERNAL_ERROR);


    log_info("(WORKER %d) [ %s ]  %-21s : %s\n", 
        worker_no, "removeFile", get_status_message(status), request->file_path);

    return status;
}

int
lock_file_handler(int worker_no, int client_fd, request_t *request)
{
    log_debug("locking file [%s]\n", request->file_path);

    int status = 0; // will be the final response status

    // Checking whether file exists
    lock_return(&(storage->access), INTERNAL_ERROR);

    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        // File doesn't exists, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
            worker_no, "lockFile", get_status_message(NOT_FOUND), request->file_path);
        
        return NOT_FOUND;
    }

    // Check if file isn't already locked
    if (!CHK_FLAG(file->flags, O_LOCK)) { 

        SET_FLAG(file->flags, O_LOCK);
        file->locked_by = client_fd;
        storage_update_file(storage, file);

        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
            worker_no, "lockFile", get_status_message(SUCCESS), request->file_path);
        
        return SUCCESS;

    } else { // File is already locked

        if (file->locked_by == client_fd) { // File is already locked by this client

            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                worker_no, "lockFile", get_status_message(SUCCESS), request->file_path);
            
            return SUCCESS;
        } else { // Otherwise adds client to list of client waiting for lock on this file
            
            int *tmp_fd = malloc(sizeof(int));
            *tmp_fd = client_fd;
            if ( list_insert_tail(file->waiting_on_lock, tmp_fd) != 0 ) {
                log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                    worker_no, "lockFile", get_status_message(INTERNAL_ERROR), request->file_path);
            
                return INTERNAL_ERROR;
            }

            // Updating file with client added to list
            storage_update_file(storage, file);

            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                            worker_no, "lockFile", get_status_message(AWAITING), request->file_path);
            
            return AWAITING; 
        } 
    }

    log_info("(WORKER %d) [  %s  ]  %-21s : %s\n", 
                            worker_no, "lockFile", get_status_message(status), request->file_path);

    return status;
}

int
unlock_file_handler(int worker_no, int client_fd, request_t *request)
{
    log_debug("unlocking file [%s]\n", request->file_path);

    int status = 0; // will be the final response status

    // Checking whether file exists
    lock_return(&(storage->access), INTERNAL_ERROR);

    file_t *file = storage_get_file(storage, request->file_path);
    if (file == NULL) {
        // File doesn't exists, log and return
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [ %s ]  %-21s : %s\n", 
            worker_no, "unlockFile", get_status_message(NOT_FOUND), request->file_path);
        
        return NOT_FOUND;
    }

    // Check whether file was previously locked
    if (!CHK_FLAG(file->flags, O_LOCK)) { 
        // File isn't locked, illegal request
        unlock_return(&(storage->access), INTERNAL_ERROR);

        log_info("(WORKER %d) [ %s ]  %-21s : %s\n", 
            worker_no, "unlockFile", get_status_message(BAD_REQUEST), request->file_path);
        
        return BAD_REQUEST;

    } else { // File was previously locked

        if (file->locked_by == client_fd) {
            // If file was locked by this client, unlock it and update
            CLR_FLAG(file->flags, O_LOCK);
            file->locked_by = -1;
            
            storage_update_file(storage, file);

            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [ %s ]  %-21s : %s\n", 
                worker_no, "unlockFile", get_status_message(SUCCESS), request->file_path);
            return SUCCESS;
        
        } else {
            // File was locked by someone else
            unlock_return(&(storage->access), INTERNAL_ERROR);

            log_info("(WORKER %d) [ %s ]  %-21s : %s\n", 
                worker_no, "unlockFile", get_status_message(UNAUTHORIZED), request->file_path);
            return UNAUTHORIZED;
        }
    }
    
    log_info("(WORKER %d) [ %s ]  %-21s : %s\n", 
        worker_no, "unlockFile", get_status_message(status), request->file_path);

    return status;
}