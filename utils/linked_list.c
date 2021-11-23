#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "linked_list.h"
#include "utilities.h"



/************** Linked List Functions **************/


list_t*
list_create(bool cmp(void*, void*), void free_fun(void*), void print(void*, FILE*))
{
    list_t *list = calloc(1, sizeof(list_t));
    if (list == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    list->length = 0;
    list->head = list->tail = NULL;

    list->cmp = (*cmp == NULL) ? default_cmp : cmp;
    list->free_fun = (*free_fun == NULL) ? default_free : free_fun;
    list->print = (*print == NULL) ? default_print : print;
    
    return list;
}

void
list_destroy(list_t *list)
{
    node_t *tmp;
    while (list->head != NULL) {
        tmp = list->head;
        list->head = list->head->next;
        list->free_fun(tmp->data);
        free(tmp);
    }
    free(list);
}

int
list_insert_tail(list_t *list, void* to_insert)
{
    if (list == NULL) {
        errno = EINVAL;
        return -1;
    }

    node_t *new = calloc(1, sizeof(node_t));
    if (new == NULL) {
        errno = ENOMEM;
        return -1;
    }

    new->data = to_insert;
    new->next = NULL;

    if (list->length == 0) { // First insertion
        list->head = list->tail = new;
    } else {
        list->tail->next = new;
        list->tail = new;
    }

    list->length++;
    return 0;
}

int 
list_insert_head(list_t *list, void* to_insert)
{
    if (list == NULL) {
        errno = EINVAL;
        return -1;
    }

    node_t *new = calloc(1, sizeof(node_t));
    if (new == NULL) {
        errno = ENOMEM;
        return -1;
    }

    new->data = to_insert;
    new->next = list->head;
    list->head = new;
    list->length++;
    return 0;
}

int
list_insert_at_index(list_t *list, void* to_insert, int index)
{
    if (list == NULL || index < 0 
        || index > list->length) {
        errno = EINVAL;
        return -1;
    }

    node_t *new = calloc(1, sizeof(node_t));
    if (new == NULL) {
        errno = ENOMEM;
        return -1;
    }

    new->data = to_insert;
    new->next = NULL;

    if (index == 0) { // Head of the list
        new->next = list->head;
        list->head = new;
        list->length++;
        return 0;
    }

    if (index == list->length) { // Tail of the list
        list->tail->next = new;
        list->tail = new;
        list->length++;
        return 0;
    }

    node_t *prev, *curr = list->head;
    while (index > 0 && curr != NULL) {
        prev = curr;
        curr = curr->next;
        index--;
    }


    new->next = curr;
    prev->next = new;
    list->length++;
    return 0;
}

int
list_remove_head(list_t *list, void** to_return)
{
    if (list == NULL || list_is_empty(list)) {
        errno = EINVAL;
        return NULL;
    }

    node_t *tmp = list->head;
    *to_return = (tmp->data);

    if (list->length == 1) { // There's only one element
        list->head = list->tail = NULL;
    } else {
        list->head = list->head->next;
    }

    free(tmp);
    list->length--;
    return tmp->data;
}

int 
list_remove_element(list_t *list, void* elem)
{
    node_t *prev = NULL, *curr = list->head;
    while (curr != NULL) {
        if (list->cmp(curr->data, elem)) break;
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL) {
        errno = ENOENT;
        return -1;
    }

    node_t *tmp = curr;

    if (prev == NULL) {
        list->head = list->head->next;
        list->free_fun(tmp->data);
        free(tmp);
    } else {
        prev->next = curr->next;
        if (prev->next == NULL) list->tail = prev;
        list->free_fun(tmp->data);
        free(tmp);
    }

    list->length--;

    return 0;
}

bool
list_is_empty(list_t *list)
{
    if (list == NULL || list->head == NULL ||
        list->length == 0) {
        return true;
    } else {
        return false;
    }
}

int
list_index_of(list_t *list, void* elem)
{
    int index = 0;
    node_t *curr = list->head;
    while (curr != NULL) {
        if (list->cmp(curr->data, elem)) return index;
        curr = curr->next;
        index++;
    }

    errno = ENOENT;
    return -1;
}

void
list_map(list_t *list, void (*fun)(void*))
{
    node_t *curr = list->head;
    while (curr != NULL) {
        fun(curr->data);
        curr = curr->next;
    }
}

void
list_dump(list_t *list, FILE *stream)
{
    node_t *curr = list->head;
    while (curr != NULL) {
        list->print(curr->data, stream);
        curr = curr->next;
    }
    fprintf(stream, "\n");
}

