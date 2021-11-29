#include "request_handlers.h"

#include "server/server_config.h"

int
open_connection_handler(int client_fd)
{
    log_debug("[OPEN CONNECTION] new client (%d)\n", client_fd);

    int status = 0; // will be the final response status

    list_t *files_opened_by_client; 
    files_opened_by_client = list_create(string_compare, free_string, string_print);
    if (files_opened_by_client == NULL) {
        log_fatal("could not create opened file list of client (%d)\n", client_fd);
        return INTERNAL_ERROR;
    }

    if (hash_map_insert(connected_clients, client_fd, files_opened_by_client) != 0) {
        log_fatal("could not create opened file list of client (%d)\n", client_fd);
        return INTERNAL_ERROR;
    }

    log_info("[OPEN CONNECTION] successfully opened new connection with client (%d)\n", client_fd);

    return status;
}

int
close_connection_handler(int client_fd)
{
    log_debug("[CLOSE CONNECTION] closing client (%d) connection\n", client_fd);

    int status = 0; // will be the final response status

    list_t *files_opened_by_client; 
    files_opened_by_client = (list_t*)hash_map_get(connected_clients, client_fd);
    if (files_opened_by_client == NULL) {
        log_fatal("client (%d) list of opened files could not be recovered\n", client_fd);
        return INTERNAL_ERROR;
    }

    while (!list_is_empty(files_opened_by_client)) {
        char *file_name = (char*)list_remove_head(files_opened_by_client);
        
        file_t *file = (file_t*)hash_map_get(storage->files, file_name);
        if (file == NULL) {

            log_debug("couldn't find file [%s] when closing client (%d) connection\n", file_name, client_fd);

        } else {
            if (CHK_FLAG(file->flags, O_LOCK) && (file->locked_by == client_fd)) {
                CLR_FLAG(file->flags, O_LOCK);
                file->locked_by = 0;
                hash_map_insert(storage->files, file->path, file);
                log_debug("client (%d) unlocked file [%s] before closing it\n", client_fd, file_name);
            }
        }
    }

    if (hash_map_remove(connected_clients, client_fd) == -1) { log_error("cannot close client %d files\n"); }
    else { log_debug("client %d successfully closed all files\n", client_fd); }
    
    log_info("[CLOSE CONNECTION] successfully closed connection with client (%d)\n", client_fd);

    return status;    
}

int
open_file_handler(int client_fd, request_t *request, list_t *expelled_files)
{
    // list_dump(storage->basic_fifo, stdout);
    log_debug("[OPEN FILE] client (%d) opening file [%s]\n", client_fd, request->resource_path);

    int status = 0; // will be the final response status

    /* Check the flags received  */
    int flags = *(int*)request->body;

    if (CHK_FLAG(flags, O_NOFLAG)) { // no flag set

        file_t *file;
        if ((file = (file_t*)hash_map_get(storage->files, request->resource_path)) == NULL) {
            return NOT_FOUND;
        }

        file->locked_by = client_fd;
        SET_FLAG(file->flags, O_NOFLAG);
        hash_map_insert(storage->files, file->path, file);

    }
    if (CHK_FLAG(flags, O_CREATE)) { // flag is O_CREATE, empty file will be created
    
        log_debug("creating file [%s]\n", request->resource_path);

        if (hash_map_get(storage->files, request->resource_path) != NULL) {
            log_error("[OPEN FILE] file [%s] already exists\n");
            return FILE_EXISTS;
        }

        file_t *new_file = calloc(1, sizeof(file_t));
        strcpy(new_file->path, request->resource_path);
        new_file->size = 0;
        new_file->locked_by = 0;
        SET_FLAG(new_file->flags, O_CREATE);
        new_file->waiting_on_lock = list_create(int_compare, NULL, print_int);

        /* Checks if a file should be expelled to make place for the new one */
        if (storage->no_of_files + 1 > storage->max_files) {
            
            char *removed_file_path = (char*)list_remove_head(storage->basic_fifo);
            file_t *removed_file = storage_remove_file(storage, removed_file_path);

            list_insert_tail(expelled_files, removed_file);
            
            log_debug("file [%s] was expelled when adding file [%s]\n", removed_file_path, request->resource_path);
            status = FILES_EXPELLED;
        }

        /* At this point we should be able to safely add the file */
        hash_map_insert(storage->files, new_file->path, new_file);
        list_insert_tail(storage->basic_fifo, new_file->path);
        storage->no_of_files++;

        log_debug("file [%s] added\n", request->resource_path);
    }
    if (CHK_FLAG(flags, O_LOCK)) { // flag is O_LOCK, if file exists it will be locked

        log_debug("locking file [%s]\n", request->resource_path);

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

        log_debug("file [%s] locked\n", request->resource_path);
    }

    /* Adds the file to list of files opened by client */
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

    return 0;

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