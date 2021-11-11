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

/**
 * \brief Creates a new hashmap. 
 * 
 * \param n_bucket: hashmap size
 * \param hash_function: function used to hash keys
 * \param key_cmp: function used to compare keys
 * \param free_fun: function used to free entries
 * \param print: function used to print entries
 * 
 * \return the hashmap on success, NULL on failure. Errno is set.
 */
hash_map_t*
hash_map_create(int n_buckets, size_t (*hash_function)(void*), bool (*key_cmp)(void*, void*), 
                void (*free_fun)(void*), void (*print)(void*, void*, FILE*));


/**
 * \brief Destroyes an hashmap, clearing all entries
 * 
 * \param hmap: hashmap to be destroyed
 * 
 * \return: 0 on success, -1 on failures. Errno is set.
 */
int
hash_map_destroy(hash_map_t *hmap);


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
hash_map_insert(hash_map_t *hmap, void* key, void* value);


/**
 * \brief Remove an entry from the hashmap
 * 
 * \param hmap: hashmap where the entry is
 * \param key: key of the entry
 * 
 * \return: 0 on success, -1 on failures. Errno is set.
 */
int 
hash_map_remove(hash_map_t *hmap, void *key);


/**
 * \brief Gets the value of an entry from the hashmap
 * 
 * \param hmap: hashmap where the entry is
 * \param key: key of the entry
 * 
 * \return: Entry value on success, NULL on failures. Errno is set.
 */
void*
hash_map_get(hash_map_t *hmap, void* key);


/**
 * \brief Prints hashmap entries to file pointer by stream
 * 
 * \param hmap: hashmap to print
 * \param stream: file pointer where to print
 */
void
hash_map_dump(hash_map_t *hmap, FILE *stream);


#endif