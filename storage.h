#ifndef STORAGE_H
#define STORAGE_H

#include "hash_map.h"
#include "linked_list.h"
#include "utilities.h"


typedef struct _file_t {

    /* File identifier */
    size_t  descriptor;
    /* Unique file path */
    char    path[MAX_PATH];
    /* File size in bytes */
    size_t  size;
    /* Pointer to file contents */
    void*   contents;   

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
    list_t *basic_fifo;

} storage_t;


/********************** Main Storage Functions  **********************/


storage_t*
storage_create(size_t max_size, size_t max_files);

int
storage_destroy(storage_t *storage);

int 
storage_open_file(storage_t *storage, int client_id, char *file_name);

int
storage_add_file(storage_t *storage, file_t *file);

int
storage_remove_file(storage_t *storage, char *file_name);

file_t*
storage_get_file(storage_t *storage, char *file_name);

void
storage_dump(storage_t *storage, FILE *stream);


/********************** Utilities for printing and deallocating a file  **********************/


void
free_file(void *e);

void
print_file(file_t *f, FILE *stream);

#endif