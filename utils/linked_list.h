#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdbool.h>
#include <stdio.h>


/************** Data Structures **************/


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

    /* The length of the list */
    int     length;
    /* Head of the list */
    node_t  *head;
    /* Tail of the list */
    node_t  *tail;
    /* Type specific compare function */
    bool    (*cmp)(void*, void*);
    /* Type specific free function */
    void    (*free_fun)(void*);
    /* Type specific print function */
    void    (*print)(void*, FILE*);

} list_t;


/************** Linked List Functions **************/

/**
 * Creates a new list, returns the list on success, NULL on failure,
 * errno is set.
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
 * errno is set.
 */
int
list_insert_head(list_t *list, void* to_insert);

/** 
 * Creates a new node and updates the list tail with said node.
 * Returns 0 on success, -1 on failure.
 * errno is set.
 */
int 
list_insert_tail(list_t *list, void* to_insert);

/**
 * Creats a new node and inserts it in the list at a certain index.
 * Returns 0 on success, -1 on failure.
 * errno is set.
 */
int
list_insert_at_index(list_t *list, void* to_insert, int index);

/**
 * Removes the current list head and copies it in to_return.
 * Return 0 on success, -1 on failure.
 * errno is set.
 */
void*
list_remove_head(list_t *list);

int
list_remove_element(list_t *list, void* elem);

/**
 * Checks whether a list contains any elements.
 * Return true if it does, false otherwise.
 * errno is set.
 */
bool
list_is_empty(list_t *list);

/**  
 * Return the index of elem in the list, if 
 * elem is not in the list it returns -1.
 * errno is set.
 */
int
list_find(list_t *list, void* elem);

/**
 * Applies fun to all the elements in the list.
 * errno is set.
 */
void
list_map(list_t *list, void fun(void*));

/**
 * Prints the list according to the type specific
 * print function to the file pointed by stream.
 * errno is set.
 */
void
list_dump(list_t *list, FILE *stream);


#endif