#include "utilities.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

size_t
int_hash(void* key) {
    int x = *(int*)key;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

size_t
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

int 
msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

int 
mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1; 
            }

            *p = '/';
        }
    }   

    if (mkdir(_path, S_IRWXU) != 0) {
        if (errno != EEXIST)
            return -1; 
    }   

    return 0;
}
 
ssize_t  /* Write "n" bytes to a descriptor */
writen(int fd, void *ptr, size_t n) {  
   size_t   nleft;
   ssize_t  nwritten;
 
   nleft = n;
   while (nleft > 0) {
     if((nwritten = write(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount written so far */
     } else if (nwritten == 0) break; 
     nleft -= nwritten;
     ptr   += nwritten;
   }
   return(n - nleft); /* return >= 0 */
}

int 
is_number(const char* s, long* n) 
{
	if (s==NULL) 
		return 1;
	if ( strlen(s) == 0) 
		return 1;
	char* e = NULL;
	errno = 0;
	long val = strtol(s, &e, 10);
	if (errno == ERANGE) 
		return 2; // overflow
	if (e != NULL && *e == (char)0) {
		*n = val;
		return 0;   // successo
		}
	return 1;   // non e' un numero
}

void
default_print(void* e, FILE* stream)
{
    fprintf(stream, "%p\n", e);
}

bool
default_cmp(void* e1, void* e2)
{
    return (e1 == e2) ? true : false;
}

void
default_free(void* ptr)
{
    return;
}

void 
string_print(void* e, FILE* stream)
{
    fprintf(stream, "%s ", (char*)e);
}

void
print_int(void* x, FILE* f) {
    fprintf(f, "%d ", *(int*)x);
}


void
free_string(void *s) {
    free(s);
}

bool 
string_compare(void* a, void* b) {
    return (strcmp( (char*)a, (char*)b ) == 0);
}

bool
int_compare(void* x, void* y) {
    return ( *(int*)x == *(int*)y );
}