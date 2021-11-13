#ifndef STORAGE_H
#define STORAGE_H

#include "hash_map.h"
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
    /* Tables of files */
    hash_map_t *files;

} storage_t;

storage_t*
storage_create(size_t max_size, size_t max_files);

int
storage_destroy(storage_t *storage);

int
storage_add_file(storage_t *storage, file_t *file);

int
storage_remove_file(storage_t *storage, char *file_name);

file_t*
storage_get_file(storage_t *storage, char *file_name);

void
storage_dump(storage_t *storage, FILE *stream);

void
free_file(void *e);

void
print_file(void *not_used, void *e, FILE *stream);

#endif