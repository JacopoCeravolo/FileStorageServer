#ifndef STORAGE_H
#define STORAGE_H

#include <errno.h>
#include <pthread.h>

#include "utils/hash_map.h"
#include "utils/linked_list.h"
#include "utils/utilities.h"


typedef struct _file_t {

    /* Unique file path */
    char                path[MAX_PATH];
    /* File flags */
    int                 flags;
    /* File size in bytes */
    size_t              size;
    /* Pointer to file contents */
    void*               contents;   
    /* Client who has the lock */
    int                 locked_by;
    /* Number of readers */
    int                 n_readers;
    /* List of client waiting for the lock */
    list_t              *waiting_on_lock;
    /* Read/write mutex */
    pthread_mutex_t     access;
    /* Condition variable */
    pthread_cond_t      available;
    

} file_t;

typedef struct _storage_t {

    /* Maximum space available */
    size_t  max_size;
    /* Current space occupied */
    size_t  current_size;
    /* Maximum number of files */
    size_t  max_files;
    /* Current number of files */
    size_t  no_of_files;
    /* Table of files */
    hash_map_t *files;
    /* Table of files currently opened */
    hash_map_t *opened_files;
    /* Very basic FIFO policy */
    list_t *fifo_queue;
    /* Read/write mutex */
    pthread_mutex_t     access;
    /* Condition variable */
    pthread_cond_t      available;


} storage_t;


/********************** Main Storage Functions  **********************/

storage_t*
storage_create(size_t max_size, size_t max_files);

int
storage_destroy(storage_t *storage);

file_t*
storage_create_file(char *file_name);

int
storage_add_file(storage_t *storage, file_t *file);

int
storage_update_file(storage_t *storage, file_t *file);

file_t*
storage_remove_file(storage_t *storage, char *file_name);

file_t*
storage_get_file(storage_t *storage, char *file_name);

int
storage_FIFO_replace(storage_t *storage, int how_many, size_t required_size, list_t *replaced_files);

void
storage_dump(storage_t *storage, FILE *stream);


/********************** Utilities for printing and deallocating a file  **********************/


void
free_file(void *e);

void
print_file(void *e, FILE *stream);

#endif