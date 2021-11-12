#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH    1024
#define MAX_NAME    64
#define MAX_BUFFER  2048

#define DEFAULT_SOCKET_PATH "/tmp/filestorage.sk"

// Run a syscall, store the result and returns ret on fail
#define SYSCALL_RETURN(result, call, msg, ret) \
        if ((result = call) == -1) { \
            int e = errno; char err_buf[1024]; strerror_r(e, &err_buf[0], 1024); \
            fprintf(stderr, "%s: %s\n", msg, err_buf); return ret; }

typedef struct _obj {
    int id;
    char *s;
} obj;


static inline bool
default_cmp(void* e1, void* e2)
{
    return (e1 == e2) ? true : false;
}

static inline void
default_free(void* ptr)
{
    return;
}

static inline void
default_print(void* e, FILE* stream)
{
    fprintf(stream, "Entry: %p\n", e);
}

static inline void
default_print_entry(void* k, void* v, FILE* stream)
{
    fprintf(stream, "Key: %p -> Value: %p\n", k, v);
}

static inline bool 
string_compare(void* a, void* b) {
    return (strcmp( (char*)a, (char*)b ) == 0);
}

static inline bool
int_compare(void* x, void* y) {
    return ( (int)x == (int)y );
}

static inline bool
obj_compare(void* x, void* y)
{
    obj *o1 = (obj*)x;
    obj *o2 = (obj*)y;
    return (o1->id == o2->id && strcmp(o1->s, o2->s) == 0);

}

static inline void 
print_string_int(void* k, void* v, FILE* stream)
{
    char *key = (char*)k;
    int value = (int)v;
    fprintf(stream, "%s -> %d\n", key, value);
}

static inline void
free_int(void* ptr) {
    return;
}

static inline void
print_int(void* x, FILE* f) {
    fprintf(f, "%d ", (int)x);
}


static inline void
free_struct(void* x)
{
    obj *o = (obj*)x;
    if(o->s) free(o->s);
}

static inline void
print_struct(void* x, FILE* f)
{
    obj *o = (obj*)x;
    fprintf(f, "%d -> %s\n", (int)o->id, (char*)o->s);
}

static inline void
increment_struct(void* x) {
    obj *o = (obj*)x;
    o->id++;
}

#endif