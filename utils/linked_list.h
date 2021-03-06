#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdbool.h>
#include <stdio.h>

/**
 * A node in a linked list
 */
typedef struct _node_t {
    void*   data;          
    struct _node_t *next;
} node_t;

/**
 * The type of a linked list
 */
typedef struct _list_t {

    int     length;
    node_t  *head;
    node_t  *tail;
    bool    (*cmp)(void*, void*);
    void    (*free_fun)(void*);
    void    (*print)(void*, FILE*);

} list_t;


/**
 * Creates a new list, returns the list on success, NULL on failure,
 */
list_t*
list_create(bool cmp(void*, void*), void free_fun(void*), void print(void*, FILE*));

/**
 * Destroys a list and all of its nodes.
 * Returns 0 on success, -1 onf failure.
 */
void
list_destroy(list_t *list);

/**
 * Creates a new node and updates the list head with said node.
 * Returns 0 on success, -1 on failure.
 */
int
list_insert_head(list_t *list, void* to_insert);

/** 
 * Creates a new node and updates the list tail with said node.
 * Returns 0 on success, -1 on failure.
 */
int 
list_insert_tail(list_t *list, void* to_insert);

/**
 * Creats a new node and inserts it in the list at a certain index.
 * Returns 0 on success, -1 on failure.
 */
int
list_insert_at_index(list_t *list, void* to_insert, int index);

/**
 * Removes the current list head.
 * Return 0 on success, -1 on failure.
 */
void*
list_remove_head(list_t *list);

/**
 * Removes the current list tail
 * Return 0 on success, -1 on failure.
 */
void*
list_remove_tail(list_t *list);

/**
 * Removes elem from list if found.
 * Return 0 on success, -1 on failure.
 */
int
list_remove_element(list_t *list, void* elem);

/**
 * Checks whether a list contains any elements.
 * Return true if it does, false otherwise.
 */
bool
list_is_empty(list_t *list);

/**  
 * Return the index of elem in the list, if 
 * elem is not in the list it returns -1.
 */
int
list_find(list_t *list, void* elem);

/**
 * Applies fun to all the elements in the list.
 */
void
list_map(list_t *list, void fun(void*));

/**
 * Return length on success, -1 on failure.
 */
int
list_length(list_t *list);

/**
 * Prints the list according to the type specific
 * print function to the file pointed by stream.
 */
void
list_dump(list_t *list, FILE *stream);



#endif