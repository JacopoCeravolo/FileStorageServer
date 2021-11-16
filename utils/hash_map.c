#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "hash_map.h"
#include "utilities.h"

/**
 * \brief Creates a new hashmap. 
 * 
 * \param n_bucket: hashmap size
 * \param hash_function: function used to hash keys
 * \param key_cmp: function used to compare keys
 * \param free_key: function used to free keys
 * \param free_value: function used to free values
 * 
 * \return the hashmap on success, NULL on failure. Errno is set.
 */
hash_map_t*
hash_map_create(int n_buckets, size_t (*hash_function)(void*), bool (*key_cmp)(void*, void*), 
                void (*free_key)(void*), void (*free_value)(void*))
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
    hmap->hash_function = (hash_function) ? hash_function : string_hash;
    hmap->key_cmp = (key_cmp) ? key_cmp : default_cmp;
    hmap->free_key = (free_key) ? free_key : default_free;
    hmap->free_value = (free_value) ? free_value : default_free;

    for (size_t i = 0; i < n_buckets; i++) {
        hmap->buckets[i] = NULL;
    }
    
    return hmap;
}


/**
 * \brief Destroyes an hashmap, clearing all entries
 * 
 * \param hmap: hashmap to be destroyed
 * 
 * \return: 0 on success, -1 on failures. Errno is set.
 */
int
hash_map_destroy(hash_map_t *hmap)
{
    for (size_t i = 0; i < hmap->n_buckets; i++)
    {
        hash_map_entry_t *entry = hmap->buckets[i], *tmp;
        while (entry != NULL) {
            tmp = entry;
            entry = entry->next;
            hmap->free_key(tmp->key);
            hmap->free_value(tmp->value);
            free(tmp);
        }
    }
    if (hmap->buckets) free(hmap->buckets);
    if (hmap) free(hmap);
    return 0;
}


/**
 * \brief Insert a new entry in the hashmap, if there's already an
 *        entry with that key, its value is replaced.
 * 
 * \param hmap: hashmap where to insert
 * \param key: key of the new entry
 * \param value: value of the new entry
 * 
 * \return: 0 on success, -1 on failures. Errno is set.
 */
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


/**
 * \brief Remove an entry from the hashmap
 * 
 * \param hmap: hashmap where the entry is
 * \param key: key of the entry
 * 
 * \returns: 0 on success, -1 on failures. Errno is set.
 */
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
        errno = ENOENT;
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


/**
 * \brief Gets the value of an entry from the hashmap
 * 
 * \param hmap: hashmap where the entry is
 * \param key: key of the entry
 * 
 * \return: Entry value on success, NULL on failures. Errno is set.
 */
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


/**
 * \brief Prints hashmap entries to file pointer by stream
 * 
 * \param hmap: hashmap to print
 * \param stream: file pointer where to print
 * \param print_key: function for printing keys
 * \param print_value: function for printing values
 */
void
hash_map_dump(hash_map_t *hmap, FILE *stream, void (*print_key)(void*, FILE*), void (*print_value)(void*, FILE*))
{
    for (size_t i = 0; i < hmap->n_buckets; i++) {    
        hash_map_entry_t *entry = hmap->buckets[i];
        while (entry != NULL) {
            print_key(entry->key, stream);
            print_value(entry->value, stream);
            entry = entry->next;
        }
    }
}
