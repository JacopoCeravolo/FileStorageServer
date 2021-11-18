#include "request_handlers.h"

#include "server/server_config.h"
#include "utils/logger.h"

int
close_connection_handler(int client_fd)
{
    log_info("client %d closing connection\n", client_fd);

    int status = 0; // will be the final response status

    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(storage->opened_files, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client %d list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    while (!list_is_empty(files_opened_by_client)) {
        char *file_name = (char*)list_remove_head(files_opened_by_client);
        file_t *file = (file_t*)hash_map_get(storage->files, file_name);
        if (file == NULL) {
            log_error("couldn't find file [%s] when closing client %d connection\n", file_name, client_fd);
        }

        if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
            CLR_FLAG(file->flags, O_LOCK);
            file->locked_by = 0;
            hash_map_insert(storage->files, file->path, file);
            log_info("client %d unlocked file [%s] before closing it\n", client_fd, file_name);
        }
    }

    hash_map_remove(storage->opened_files, client_fd);
    log_info("client %d successfully closed all files\n", client_fd);
    return status;    
}
int
open_file_handler(int client_fd, request_t *request, list_t *expelled_files)
{
    log_info("client %d opening file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    /* Check the flags received  */
    int flags = *(int*)request->body;

    if (CHK_FLAG(flags, O_CREATE)) { // flag is O_CREATE, empty file will be created
    
        log_info("creating file [%s]\n", request->resource_path);

        if (hash_map_get(storage->files, request->resource_path) != NULL) {
            log_error("file [%s] already exists\n");
            return FILE_EXISTS;
        }

        file_t *new_file = calloc(1, sizeof(file_t));
        strcpy(new_file->path, request->resource_path);
        new_file->size = 0;
        new_file->locked_by = 0;
        SET_FLAG(new_file->flags, O_CREATE);

        /* Checks if a file should be expelled to make place for the new one */
        if (storage->no_of_files + 1 > storage->max_files) {

            char *removed_file_path = (char*)list_remove_head(storage->basic_fifo);
            file_t *removed_file = storage_remove_file(storage, removed_file_path);
            list_insert_tail(expelled_files, removed_file);
            
            log_info("file [%s] was expelled when adding file [%s]\n", removed_file_path, request->resource_path);
            status = FILES_EXPELLED;
        }

        /* At this point we should be able to safely add the file */
        hash_map_insert(storage->files, new_file->path, new_file);
        list_insert_tail(storage->basic_fifo, new_file->path);
        storage->no_of_files++;

        log_info("file [%s] added\n", request->resource_path);
    }
    if (CHK_FLAG(flags, O_LOCK)) { // flag is O_LOCK, if file exists it will be locked

        log_info("locking file [%s]\n", request->resource_path);

        file_t *file;
        if ((file = (file_t*)hash_map_get(storage->files, request->resource_path)) == NULL) {
            return NOT_FOUND;
        }

        if (file->locked_by != 0) {

            log_warning("file [%s] is locked by client %d\n", file->path,file->locked_by);
            return MISSING_BODY; // for me to remember to implement lock queue
        }

        file->locked_by = client_fd;
        SET_FLAG(file->flags, O_LOCK);
        hash_map_insert(storage->files, file->path, file);

        log_info("file [%s] locked\n", request->resource_path);
    }

    /* Adds the file to list of files opened by client */
    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(storage->opened_files, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client %d list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    char *filename = calloc(1, MAX_NAME);
    strcpy(filename, request->resource_path);
    list_insert_tail(files_opened_by_client, filename);
    hash_map_insert(storage->opened_files, client_fd, files_opened_by_client);
    log_info("file [%s] successfully opened by client %d\n", filename, client_fd);
    return status;
}

int
close_file_handler(int client_fd, request_t *request)
{
    log_info("client %d closing file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    file_t *file = (file_t*)hash_map_get(storage->files, request->resource_path);
    if (file == NULL) {
        log_error("file [%s] doesn't exists\n", request->resource_path);
        return NOT_FOUND;
    }

    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(storage->opened_files, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client %d list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    if (list_index_of(files_opened_by_client, request->resource_path) == -1) {
        log_error("client %d must open file [%s] before closing it\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
        CLR_FLAG(file->flags, O_LOCK);
        file->locked_by = 0;
        hash_map_insert(storage->files, file->path, file);
        log_info("client %d unlocked file [%s] before closing it\n", client_fd, request->resource_path);
    }

    list_remove_element(files_opened_by_client, request->resource_path);
    hash_map_insert(storage->opened_files, client_fd, files_opened_by_client);
    log_info("file [%s] successfully closed by client %d\n", request->resource_path, client_fd);
    return status;
}

int
write_file_handler(int client_fd, request_t *request, list_t *expelled_files)
{
    log_info("client %d writing file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    /* Checks if the request contains any content */
    if (request->body_size == 0 || request->body == NULL) {
        log_error("client %d bad write request\n", client_fd);
        return MISSING_BODY;
    }

    /* Creates the file */
    file_t *file = storage_get_file(storage, client_fd, request->resource_path);
    
    if (file == NULL) {
        log_error("file [%s] doesn't exists\n", request->resource_path);
        return NOT_FOUND;
    }

    // SHOULD ALSO CHECK THE FLAG
    if (file->locked_by != client_fd) {
        log_error("client %d must lock file [%s] before writing\n", client_fd, request->resource_path);
        return UNAUTHORIZED;
    }

    if (!CHK_FLAG(file->flags, O_CREATE)) {
        log_error("file [%s] exists and cannot be rewritten\n", request->resource_path);
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
    log_info("file [%s] successfully written by client %d\n", request->resource_path, client_fd);
    return SUCCESS;
            
}

/* not good */
int
read_file_handler(int client_fd, request_t *request)
{
    int status = 0;
    file_t *file;
    file = storage_get_file(storage, client_fd, request->resource_path);
    if (file == NULL) return NOT_FOUND;

    return SUCCESS;
}

int
remove_file_handler(int client_fd, request_t *request)
{
    int status = 0;
    file_t *to_remove;
    if ((to_remove = storage_remove_file(storage, request->resource_path)) == NULL) {
        switch (errno) {
            // Will be more
            case ENOENT: {
                // log_error("[%s] file (%s) could not be found\n", worker_name, request->resource_path);
                return NOT_FOUND; 
            }
        }
    } else {
        // free_file(to_remove);
    }
    return status;
}

