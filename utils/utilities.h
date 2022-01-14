#ifndef UTILITIES_H
#define UTILITIES_H

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

#define O_NOFLAG    0x1
#define O_CREATE    0x2
#define O_LOCK      0x4

#define SET_FLAG(n, f) ((n) |= (f)) 
#define CLR_FLAG(n, f) ((n) &= ~(f)) 
#define TGL_FLAG(n, f) ((n) ^= (f)) 
#define CHK_FLAG(n, f) ((n) & (f))

#define MAX_PATH    1024

#define lock_return(mtx, ret) { \
            if(pthread_mutex_lock(mtx) != 0) { \
                fprintf(stderr, "pthread_mutex_lock failed: %s", strerror(errno)); return ret; }}

#define unlock_return(mtx, ret) { \
            if(pthread_mutex_unlock(mtx) != 0) { \
                fprintf(stderr,"pthread_mutex_unlock failed: %s", strerror(errno)); return ret; }}

#define cond_wait_return(cond, mtx, ret) { \
            if(pthread_cond_wait(cond, mtx) != 0) { \
                fprintf(stderr,"pthread_cond_wait failed: %s", strerror(errno)); return ret; }}

#define cond_signal_return(cond, ret) { \
            if(pthread_cond_signal(cond) != 0) { \
                fprintf(stderr,"pthread_cond_signal failed: %s", strerror(errno)); return ret; }}

#define cond_broadcast_return(cond, ret) { \
            if(pthread_cond_broadcast(cond) != 0) { \
                fprintf(stderr,"pthread_cond_broadcast failed: %s", strerror(errno)); return ret; }}


void default_print(void* e, FILE* stream);

bool default_cmp(void* e1, void* e2);

void default_free(void* ptr);

void string_print(void* e, FILE* stream);

void print_int(void* x, FILE* f);


void free_string(void *s);

bool string_compare(void* a, void* b);

bool int_compare(void* x, void* y);


size_t int_hash(void* key);

ssize_t readn(int fd, void *ptr, size_t n);

ssize_t writen(int fd, void *ptr, size_t n);

size_t string_hash(void* key);


int msleep(long msec);

int mkdir_p(const char *path);

int 
is_number(const char* s, long* n);

#endif
