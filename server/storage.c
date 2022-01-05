#include <errno.h>

#include "server/storage.h"
#include "utils/utilities.h"
#include "server/logger.h"

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

    /* storage->opened_files = hash_map_create(500, int_hash, int_compare, NULL, list_destroy);
    if (storage->opened_files == NULL) {
        hash_map_destroy(storage->files);
        free(storage);
        errno = ENOMEM;
        return NULL;
    } */

    storage->basic_fifo = list_create(string_compare, NULL, string_print);
    if (storage->basic_fifo == NULL) {
        hash_map_destroy(storage->files);
        // hash_map_destroy(storage->opened_files);
        free(storage);
        errno = ENOMEM;
        return NULL;
    }

    if ((pthread_mutex_init(&(storage->access), NULL)) != 0 ||
        (pthread_cond_init(&(storage->available), NULL) != 0)) {
        return NULL;
    }

    return storage;
}

int
storage_destroy(storage_t *storage)
{
    log_debug("deallocating file table\n");
    if (storage->files) hash_map_destroy(storage->files);
    log_debug("deallocating FIFO queue\n");
    list_destroy(storage->basic_fifo);
    log_debug("deallocating storage unit\n");
    if (storage) free(storage);
    return 0;
}

/* file_t*
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
 */
file_t*
storage_create_file(char *file_name)
{  
    file_t *new_file = calloc(1, sizeof(file_t));
    if (new_file == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    strcpy(new_file->path, file_name);
    new_file->size = 0;
    new_file->locked_by = -1;
    SET_FLAG(new_file->flags, O_CREATE);
    new_file->waiting_on_lock = list_create(int_compare, NULL, NULL);
    if ((pthread_mutex_init(&(new_file->access), NULL)) != 0 ||
        (pthread_cond_init(&(new_file->available), NULL) != 0)) {
        return NULL;
    }

    return new_file;
}

int
storage_add_file(storage_t *storage, file_t *file)
{
    hash_map_insert(storage->files, file->path, file);
    list_insert_tail(storage->basic_fifo, file->path);
    storage->no_of_files++;
}

int
storage_update_file(storage_t *storage, file_t *file)
{
    hash_map_insert(storage->files, file->path, file);
}

file_t*
storage_remove_file(storage_t *storage, char *file_name)
{
    file_t *to_remove = (file_t*)hash_map_get(storage->files, file_name);
    storage->no_of_files--;
    storage->current_size = storage->current_size - to_remove->size;
    list_remove_element(storage->basic_fifo, to_remove->path);
    hash_map_remove(storage->files, to_remove->path);
    return to_remove;
}

file_t*
storage_get_file(storage_t *storage, char *file_name)
{
    if (storage == NULL || file_name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    file_t *file = (file_t*)hash_map_get(storage->files, (void*)file_name);
    if (file == NULL) return NULL;
    return file;
}

int
storage_FIFO_replace(storage_t *storage, int how_many, size_t required_size, list_t *replaced_files)
{
    /* This all should be in a separate function */

    char   *removed_file_path;
    file_t *to_remove;
    int files_removed = 0;

    while ( storage->current_size + required_size > storage->max_size ) {
        
        removed_file_path = (char*)list_remove_head(storage->basic_fifo);
        to_remove = storage_get_file(storage, removed_file_path);

        // if (CHK_FLAG(to_remove->flags, O_CREATE)) continue;

        file_t *copy = calloc(1, sizeof(file_t));
        strcpy(copy->path, to_remove->path);
        copy->size = to_remove->size;
        copy->contents = malloc(copy->size);
        memcpy(copy->contents, to_remove->contents, copy->size);

        storage->current_size = storage->current_size - to_remove->size;
        storage->no_of_files--;
        hash_map_remove(storage->files, to_remove->path);

        list_insert_tail(replaced_files, copy);
        files_removed++;
    }

    return files_removed;
    /* ************************** */
}


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
    if (f->contents) free(f->contents);
    list_destroy(f->waiting_on_lock);
    free(f);
}

void
print_file(void *e, FILE *stream)
{
    file_t *f = (file_t*)e;
    fprintf(stream, " %lu (bytes)", f->size);
    fprintf(stream, " locked by (%d)\n", f->locked_by);
    fprintf(stream, "Clients waiting for lock: ");
    // list_dump(f->waiting_on_lock, stream);
    fprintf(stream, "\n");
    fprintf(stream, "\n------------------------------------------------------\n\n");
}