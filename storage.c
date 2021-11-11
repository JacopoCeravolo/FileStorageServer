#include <errno.h>

#include "storage.h"

storage_t*
storage_create(size_t max_size, size_t max_files)
{
    storage_t *storage = malloc(sizeof(storage_t));
    if (storage == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    storage->max_size = max_size;
    storage->max_files = max_files;
    storage->current_size = 0;
    storage->no_of_files = 0;

    storage->files = hash_map_create((max_files * 2), NULL, string_compare, free_file, print_file);
    if (storage->files == NULL) {
        free(storage);
        errno = ENOMEM;
        return NULL;
    }

    return storage;
}

int
storage_destroy(storage_t *storage)
{
    if (storage->files) hash_map_destroy(storage->files);
    if (storage) free(storage);
    return 0;
}

int
storage_add_file(storage_t *storage, file_t *file)
{
    if (storage == NULL || file == NULL) {
        errno = EINVAL;
        return -1;
    }

    storage->current_size =+ file->size;
    storage->no_of_files++;
    return hash_map_insert(storage->files, file->path, (void*)file);
}

int
storage_remove_file(storage_t *storage, char *file_name)
{
    return 0;
}

file_t*
storage_get_file(storage_t *storage, char *file_name)
{
    if (storage == NULL || file_name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    return (file_t*)hash_map_get(storage->files, (void*)file_name);
}

void
storage_dump(storage_t *storage, FILE *stream)
{
    fprintf(stream, "Current Size: %lu\n", storage->current_size);
    fprintf(stream, "No Of Files: %lu\n", storage->no_of_files);
    hash_map_dump(storage->files, stream);
}

void
free_file(void *e) 
{
    file_t *f = (file_t*)e;
    free(f->contents);
    free(f);
}

void
print_file(void *not_used, void *e, FILE *stream)
{
    file_t *f = (file_t*)e;
    fprintf(stream,
    "\n**********************\n"
    "* Name: %s\n"
    "* Size: %lu\n"
    "* Contents:\n%s\n"
    "**********************\n", f->path, f->size, (char*)f->contents);
}