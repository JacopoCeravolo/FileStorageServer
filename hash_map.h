#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdbool.h>
#include <stdio.h>


typedef struct _hash_map_entry_t {

    void* key;
    void* value;
    struct _hash_map_entry_t *next;

} hash_map_entry_t;

typedef struct _hash_map_t {

    int n_buckets;
    int n_entries;
    hash_map_entry_t **buckets;
    /* Hash function to be used */
    size_t (*hash_function)(void*);
    /* Type specific compare function */
    bool    (*key_cmp)(void*, void*);
    /* Type specific free function */
    void    (*free_fun)(void*);
    /* Type specific print function */
    void    (*print)(void*, void*, FILE*);

} hash_map_t;

hash_map_t*
hash_map_create(int n_buckets, size_t (*hash_function)(void*), bool (*key_cmp)(void*, void*), 
                void (*free_fun)(void*), void (*print)(void*, void*, FILE*));

int
hash_map_destroy(hash_map_t *hmap);

int
hash_map_insert(hash_map_t *hmap, void* key, void* value);

int 
hash_map_remove(hash_map_t *hmap, void *key);

void*
hash_map_get(hash_map_t *hmap, void* key);

void
hash_map_dump(hash_map_t *hmap, FILE *stream);


#endif