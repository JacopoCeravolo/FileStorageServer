#include <errno.h>
#include <stdlib.h>
#include "server/storage.h"
#include "utils/utilities.h"
#include "server/logger.h"

storage_t*
storage_create(size_t max_size, size_t max_files)
{
    // Allocating space for storage
    storage_t *storage = calloc(1, sizeof(storage_t));
    if (storage == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // Setting initial configuration
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

    storage->fifo_queue = list_create(string_compare, free_string, string_print);
    if (storage->fifo_queue == NULL) {
        hash_map_destroy(storage->files);
        free(storage);
        errno = ENOMEM;
        return NULL;
    }

    if ((pthread_mutex_init(&(storage->access), NULL)) != 0) {
        return NULL;
    }

    return storage;
}

int
storage_destroy(storage_t *storage)
{
    hash_map_destroy(storage->files);
    list_destroy(storage->fifo_queue);
    free(storage);
    return 0;
}

file_t*
storage_create_file(char *file_name)
{  
    // Allocating file
    file_t *new_file = calloc(1, sizeof(file_t));
    if (new_file == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // Setting initial file fileds
    strcpy(new_file->path, file_name);
    new_file->size = 0;
    new_file->locked_by = -1;
    SET_FLAG(new_file->flags, O_CREATE);
    new_file->waiting_on_lock = list_create(NULL, NULL, NULL);
    
    return new_file;
}

int
storage_add_file(storage_t *storage, file_t *file)
{
    // Adds the file to storage data structures
    if ( hash_map_insert(storage->files, file->path, file) != 0 ) return -1;
    // if ( list_insert_tail(storage->fifo_queue, file->path) != 0 ) return -1;
    storage->no_of_files++;
    return 0;
}

int
storage_update_file(storage_t *storage, file_t *file)
{
    return hash_map_insert(storage->files, file->path, file);
}

int
storage_remove_file(storage_t *storage, char *file_name)
{
    // Finds the file
    file_t *to_remove = (file_t*)hash_map_get(storage->files, file_name);
    if (to_remove == NULL) return -1;
    
    // Update storage fields and data structures
    storage->no_of_files--;
    storage->current_size = storage->current_size - to_remove->size;
    /* char *filename1 = malloc(strlen(file_name) + 1);
    strcpy(filename1, file_name);
    char *filename2 = malloc(strlen(file_name) + 1);
    strcpy(filename2, file_name); */
    if ( list_remove_element(storage->fifo_queue, file_name) != 0 ) return -1;
    if ( hash_map_remove(storage->files, file_name) != 0 ) return -1;
    return 0;
}

file_t*
storage_get_file(storage_t *storage, char *file_name)
{
    // Finds the file in hashtable
    file_t *file = (file_t*)hash_map_get(storage->files, (void*)file_name);
    if (file == NULL) return NULL;
    return file;
}

int
storage_FIFO_replace(storage_t *storage, int how_many, size_t required_size, list_t *replaced_files)
{

    char   *removed_file_path;
    file_t *to_remove;
    int files_removed = 0;

    while ( storage->current_size + required_size > storage->max_size ) {
        
        // Dequeue filename from FIFO queue
        removed_file_path = (char*)list_remove_head(storage->fifo_queue);
        to_remove = storage_get_file(storage, removed_file_path);


        // Copies file contents
        file_t *copy = calloc(1, sizeof(file_t));
        strcpy(copy->path, to_remove->path);
        copy->size = to_remove->size;
        copy->contents = malloc(copy->size);
        memcpy(copy->contents, to_remove->contents, copy->size);

        // Removes file from storage
        storage->current_size = storage->current_size - to_remove->size;
        storage->no_of_files--;
        hash_map_remove(storage->files, to_remove->path);

        // Adds file to list of expelled files
        list_insert_tail(replaced_files, copy);
        files_removed++;
    }

    return files_removed;
    
}

void
storage_dump(storage_t *storage, FILE *stream)
{
    fprintf(stream, "\n**********************\n");
    fprintf(stream, "CURRENT SIZE: %lu", storage->current_size);
    fprintf(stream, "\n**********************\n");

    fprintf(stream, "\n**********************\n");
    fprintf(stream, "NO OF FILES: %d", storage->no_of_files);
    fprintf(stream, "\n**********************\n");

    fprintf(stream, "\n**********************\n");
    fprintf(stream, "FIFO QUEUE\n");
    list_dump(storage->fifo_queue, stream);
    fprintf(stream, "\n**********************\n");

    fprintf(stream, "\n**** TABLE OF FILES ******\n\n");
    hash_map_dump(storage->files, stream, string_print, print_file);
}

void
free_file(void *e) 
{
    file_t *f = (file_t*)e;
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
    list_dump(f->waiting_on_lock, stream);
    fprintf(stream, "\n");
    fprintf(stream, "\n------------------------------------------------------\n\n");
}