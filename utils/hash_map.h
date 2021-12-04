#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdbool.h>
#include <stdio.h>

#define HASH_MAP_GET_NEXT(key, value) \
            do { (key) = }


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
    void    (*free_key)(void*);
    /* Type specific print function */
    void    (*free_value)(void*);

} hash_map_t;


/********************** Main Hash Map Functions  **********************/


hash_map_t*
hash_map_create(int n_buckets, size_t (*hash_function)(void*), bool (*key_cmp)(void*, void*), 
                void (*free_key)(void*), void (*free_value)(void*));

int
hash_map_destroy(hash_map_t *hmap);

int
hash_map_insert(hash_map_t *hmap, void* key, void* value);

int 
hash_map_remove(hash_map_t *hmap, void *key);

void*
hash_map_get(hash_map_t *hmap, void* key);

hash_map_entry_t*
hash_map_get_entry(hash_map_t *hmap, int index);


void
hash_map_dump(hash_map_t *hmap, FILE *stream, void (*print_key)(void*, FILE*), void (*print_value)(void*, FILE*));

#define hashmap_foreach(hmap, index, entry, key, value)                                             \
            for (index = 0; index < hmap->n_buckets; index++)                                       \
                for (entry = hmap->buckets[i];                                                      \
                    (entry != NULL) && ((key=entry->key)!= NULL) && ((data=entry->data)!= NULL);    \
                    entry = entry->next)                                                            

#endif