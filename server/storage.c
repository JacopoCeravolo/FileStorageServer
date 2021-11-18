#include <errno.h>

#include "server/storage.h"
#include "utils/utilities.h"
#include "utils/logger.h"

/**
 * \brief Initializes the storage unit with maximum capacity and maximum number of files.
 * 
 * \param max_size: total maximum capacity
 * \param max_files: total number of files
 * 
 * \return the new storage unit on success, NULL on failure. Errno is set.
 */
storage_t*
storage_create(size_t max_size, size_t max_files)
{
    storage_t *storage = calloc(1, sizeof(storage_t));
    if (storage == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    storage->max_size = max_size;
    storage->max_files = max_files;
    storage->current_size = 0;
    storage->no_of_files = 0;

    storage->files = hash_map_create((max_files * 2), string_hash, string_compare, NULL, free_file);
    if (storage->files == NULL) {
        free(storage);
        errno = ENOMEM;
        return NULL;
    }

    storage->opened_files = hash_map_create(500, int_hash, int_compare, NULL, list_destroy);
    if (storage->opened_files == NULL) {
        hash_map_destroy(storage->files);
        free(storage);
        errno = ENOMEM;
        return NULL;
    }

    storage->basic_fifo = list_create(string_compare, NULL, string_print);
    if (storage->basic_fifo == NULL) {
        hash_map_destroy(storage->files);
        hash_map_destroy(storage->opened_files);
        free(storage);
        errno = ENOMEM;
        return NULL;
    }

    return storage;
}


/** 
 * \brief Deallocates the storage and its components
 * 
 * \param storage: the storage unit to destory
 * 
 * \return 0 on success, -1 on failure. Errno is set.
 */
int
storage_destroy(storage_t *storage)
{
    log_debug("deallocating file table\n");
    if (storage->files) hash_map_destroy(storage->files);
    log_debug("deallocating client table\n");
    if (storage->opened_files) hash_map_destroy(storage->opened_files);
    log_debug("deallocating FIFO queue\n");
    if (storage->basic_fifo) list_destroy(storage->basic_fifo);
    log_debug("deallocating storage unit\n");
    if (storage) free(storage);
    return 0;
}

/**
 * \brief Removes file associated with \param file_name from storage unit
 * 
 * \param storage: the storage unit
 * \param file_name: file to be removed
 * 
 * \return 0 on success, -1 on failure. Errno is set.
 */
file_t*
storage_remove_file(storage_t *storage, char *file_name)
{
    file_t *to_remove = (file_t*)hash_map_get(storage->files, file_name);
    if (to_remove == NULL) {
        errno = ENOENT;
        return NULL;
    }

    list_remove_element(storage->basic_fifo, file_name);
    if (hash_map_remove(storage->files, file_name) != 0) return NULL;
    storage->no_of_files--;
    storage->current_size = storage->current_size - to_remove->size;
    return to_remove;
}


/** 
 * \brief Search the storage unit for the file associated with \param file_name
 * 
 * \param storage: the storage unit
 * \param file_name: requested file's name
 * 
 * \return file associated with \param file_name on success, NULL on failure. Errno is set.
 */
file_t*
storage_get_file(storage_t *storage, int client_id, char *file_name)
{
    if (storage == NULL || file_name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    return (file_t*)hash_map_get(storage->files, (void*)file_name);
}

int
storage_FIFO_replace(storage_t *storage, int how_many, size_t required_size, list_t *replaced_files)
{
    /* This all should be in a separate function */

    char   *removed_file_path;
    file_t *removed_file;
    int files_removed = 0;

    while ( (storage->no_of_files + how_many > storage->max_files) ||
            (storage->current_size + required_size > storage->max_size) ) {
        
        removed_file_path = list_remove_head(storage->basic_fifo);
        removed_file = storage_remove_file(storage, removed_file_path);
        list_insert_tail(replaced_files, removed_file);
        files_removed++;
        log_debug("file [%s] was deleted during creation\n", removed_file_path);
    }

    return files_removed;
    /* ************************** */
}


/**
 * \brief Prints the storage unit components to the file pointed by \param stream
 * 
 * \param storage: the storage unit
 * \param stream: file pointer 
 */
void
storage_dump(storage_t *storage, FILE *stream)
{
    fprintf(stream, "\n**********************\n");
    fprintf(stream, "CURRENT SIZE: %lu", storage->current_size);
    fprintf(stream, "\n**********************\n");

    fprintf(stream, "\n**********************\n");
    fprintf(stream, "NO OF FILES: %lu", storage->no_of_files);
    fprintf(stream, "\n**********************\n");

    fprintf(stream, "\n**********************\n");
    fprintf(stream, "OPENED FILES MAP\n");
    hash_map_dump(storage->opened_files, stream, print_int, list_dump);
    fprintf(stream, "\n**********************\n");

    fprintf(stream, "\n**********************\n");
    fprintf(stream, "FIFO QUEUE\n");
    list_dump(storage->basic_fifo, stream);
    fprintf(stream, "\n**********************\n");

    fprintf(stream, "\n**** TABLE OF FILES ******\n\n");
    hash_map_dump(storage->files, stream, string_print, print_file);
}



/********************** Utilities for printing and deallocating a file  **********************/

void
free_file(void *e) 
{
    file_t *f = (file_t*)e;
    log_debug("deallocating file (%s)\n", f->path);
    free(f->contents);
    list_destroy(f->waiting_on_lock);
    free(f);
}

void
print_file(file_t *f, FILE *stream)
{
    fprintf(stream,
    " %lu (bytes)\n", f->size);
    fprintf(stream, "\n------------------------------------------------------\n");
    fprintf(stream, "Clients waiting for lock: ");
    list_dump(f->waiting_on_lock, stream);
    fprintf(stream, "\n");
    /* fprintf(stream,
    "%lu (bytes)\n\n%s", f->size, (char*)f->contents); */
    fprintf(stream, "\n------------------------------------------------------\n\n");
}