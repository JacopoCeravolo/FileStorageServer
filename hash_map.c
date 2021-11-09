#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "hash_map.h"

#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

/************** Generic compare, free and print **************/


bool
default_cmp(void* e1, void* e2)
{
    return (e1 == e2) ? true : false;
}

void
default_free(void* ptr)
{
    if (ptr) free(ptr);
}

void
default_print(void* k, void* v, FILE* stream)
{
    fprintf(stream, "Key: %p -> Value: %p\n", k, v);
}


void
print_entry(hash_map_entry_t *e, FILE* stream)
{
    fprintf(stream, "%s -> %d\n", (char*)e->key, (int)e->value);
}

/************** Default Hash Function **************/


size_t
hash_pjw(void* key)
{
    char *datum = (char *)key;
    size_t hash_value, i;

    if(!datum) return 0;

    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ONE_EIGHTH) + *datum;
        if ((i = hash_value & HIGH_BITS) != 0)
            hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
    return (hash_value);
}

/************** Hash Map Functions **************/


hash_map_t*
hash_map_create(int n_buckets, size_t (*hash_function)(void*), bool (*key_cmp)(void*, void*), 
                void (*free_fun)(void*), void (*print)(void*, FILE*))
{
    hash_map_t *hmap = malloc(sizeof(hash_map_t));
    if (hmap == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    hmap->buckets = calloc(n_buckets, sizeof(hmap->buckets));
    if (hmap->buckets == NULL) {
        free(hmap);
        errno = ENOMEM;
        return NULL;
    }

    hmap->n_buckets = n_buckets;
    hmap->n_entries = 0;
    hmap->hash_function = (hash_function) ? hash_function : hash_pjw;
    hmap->key_cmp = (key_cmp) ? key_cmp : default_cmp;
    hmap->free_fun = (free_fun) ? free_fun : default_free;
    hmap->print = (print) ? print : default_print;

    for (size_t i = 0; i < n_buckets; i++) {
        hmap->buckets[i] = NULL;
    }
    
    return hmap;
}

int
hash_map_destroy(hash_map_t *hmap)
{
    for (size_t i = 0; i < hmap->n_buckets; i++)
    {
        hash_map_entry_t *entry = hmap->buckets[i], *tmp;
        while (entry != NULL) {
            tmp = entry;
            entry = entry->next;
            free(tmp);
        }
    }
    if (hmap->buckets) free(hmap->buckets);
    if (hmap) free(hmap);
}

int
hash_map_insert(hash_map_t *hmap, void* key, void* value)
{
    size_t hashed_key = hmap->hash_function(key) % hmap->n_buckets;
    hash_map_entry_t *entry = hmap->buckets[hashed_key], *prev = NULL;


    while (entry != NULL) {
        if (hmap->key_cmp(entry->key, key)) break;
        prev = entry;
        entry = entry->next;
    }

    if (entry == NULL) {
        entry = malloc(sizeof(hash_map_entry_t));
        if (entry == NULL) {
            errno = ENOMEM;
            return -1;
        }

        entry->key = key;
        entry->value = value;
        entry->next = NULL;

        if (prev == NULL) {
            hmap->buckets[hashed_key] = entry;
        } else {
            prev->next = entry;
        }
    } else {
        entry->value = value;
    }

    return 0;
}

int 
hash_map_remove(hash_map_t *hmap, void *key)
{
    size_t hashed_key = hmap->hash_function(key) % hmap->n_buckets;
    hash_map_entry_t *entry = hmap->buckets[hashed_key], *prev = NULL;

    while (entry != NULL) {
        if (hmap->key_cmp(entry->key, key)) {
            prev = entry;
            break;
        }
        entry = entry->next;
    }

    if (entry == NULL) {
        // errno?
        return -1;
    }

    if (prev == NULL) {
        hmap->buckets[hashed_key] = entry->next;
    } else {
        prev->next = entry->next;
    }

    hmap->buckets[hashed_key] = NULL;
    free(entry);
    return 0;
}

void*
hash_map_get(hash_map_t *hmap, void* key)
{
    size_t hashed_key = hmap->hash_function(key) % hmap->n_buckets;
    hash_map_entry_t *entry = hmap->buckets[hashed_key];

    while (entry != NULL) {
        if (hmap->key_cmp(entry->key, key)) return entry->value;
        entry = entry->next;
    }

    return NULL;
}

void
hash_map_dump(hash_map_t *hmap, FILE *stream)
{
    for (size_t i = 0; i < hmap->n_buckets; i++)
    {
        
        hash_map_entry_t *entry = hmap->buckets[i];
        while (entry != NULL) {
            hmap->print(entry->key, entry->value, stream);
            entry = entry->next;
        }
    }
}
