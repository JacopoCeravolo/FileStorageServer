#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/********************** Constant Macros  **********************/

#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

#define O_CREATE    (1 << 0)
#define O_LOCK      (1 << 1)
#define O_READ      (1 << 2)

#define SET_FLAG(n, f) ((n) |= (f)) 
#define CLR_FLAG(n, f) ((n) &= ~(f)) 
#define TGL_FLAG(n, f) ((n) ^= (f)) 
#define CHK_FLAG(n, f) ((n) & (f))

#define MAX_PATH    1024
#define MAX_NAME    64
#define MAX_BUFFER  2048

#define DEFAULT_SOCKET_PATH "/tmp/filestorage.sk"


/********************** Function Macros  **********************/

// Run a syscall, store the result and returns ret on fail
#define SYSCALL_RETURN(result, call, msg, ret) \
        if ((result = call) == -1) { \
            int e = errno; char err_buf[1024]; strerror_r(e, &err_buf[0], 1024); \
            fprintf(stderr, "%s: %s\n", msg, err_buf); return ret; }


/********************** Generic print, compare and free functions  **********************/

static inline void
default_print(void* e, FILE* stream)
{
    fprintf(stream, "%p\n", e);
}

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

/********************** Type Specific Print Functions  **********************/


static inline void 
string_print(void* e, FILE* stream)
{
    fprintf(stream, "%s ", (char*)e);
}

static inline void
print_int(void* x, FILE* f) {
    fprintf(f, "%d ", (int)x);
}


/********************** Type Specific Free Functions  **********************/

static inline void
free_string(void *s) {
    free(s);
}

/********************** Type Specific Compare Functions  **********************/


static inline bool 
string_compare(void* a, void* b) {
    return (strcmp( (char*)a, (char*)b ) == 0);
}

static inline bool
int_compare(void* x, void* y) {
    return ( (int)x == (int)y );
}

/********************** Hashing Functions  **********************/

static inline size_t
int_hash(void* key) {
    int x = (int)key;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

static inline size_t
string_hash(void* key)
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



#endif