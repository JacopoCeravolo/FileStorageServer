#ifndef STORAGE_H
#define STORAGE_H

#include <errno.h>
#include <pthread.h>

#include "utils/hash_map.h"
#include "utils/linked_list.h"
#include "utils/utilities.h"

/**
 * A file in storage
 */
typedef struct _file_t {
    char    path[MAX_PATH];
    int     flags;
    size_t  size;
    void*   contents;   
    int     locked_by;
    list_t  *waiting_on_lock;
} file_t;

/**
 * The storage
 */
typedef struct _storage_t {
    size_t          max_size;
    size_t          current_size;
    int             max_files;
    int             no_of_files;
    hash_map_t      *files;
    list_t          *fifo_queue;
    pthread_mutex_t access;
} storage_t;



/**
 * Allocates storage
 */
storage_t*
storage_create(size_t max_size, size_t max_files);

/**
 * Deallocates storage
 */
int
storage_destroy(storage_t *storage);

/**
 * Creates a new file
 */
file_t*
storage_create_file(char *file_name);

/**
 * Adds the file to storage
 */
int
storage_add_file(storage_t *storage, file_t *file);

/**
 * Update file contents in storage
 */
int
storage_update_file(storage_t *storage, file_t *file);

/**
 * Remove file from storage
 */
int
storage_remove_file(storage_t *storage, char *file_name);

/**
 * Find a file in storage
 */
file_t*
storage_get_file(storage_t *storage, char *file_name);

/**
 * Replace files up to how_many or until there is enough space
 */
int
storage_FIFO_replace(storage_t *storage, int how_many, size_t required_size, list_t *replaced_files);

/**
 * Prints storage 
 */
void
storage_dump(storage_t *storage, FILE *stream);

/**
 * Deallocates a file
 */
void
free_file(void *e);

/**
 * Print a file
 */
void
print_file(void *e, FILE *stream);

#endif