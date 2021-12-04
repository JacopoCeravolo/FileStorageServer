#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H


#include <stdio.h>
#include <stdlib.h>

#include "utils/linked_list.h"

#define CLIENT_OPTIONS  "w:W:r:R:l:u:c:f:d:D:ht:p"
#define MAX_ARG_LENGTH  128

typedef enum {
    WRITE,
    READ,
    WRITE_DIR,
    READ_N,
    LOCK,
    UNLOCK,
    REMOVE
} action_code;

typedef struct _action_t {
    action_code code;
    char        *arguments;
    char        *directory;
    long        wait_time;
} action_t;

void
print_help_msg();

int
parse_options_in_list(int n_options, char * const option_vector[], list_t *action_list);

void
execute_action(action_t *action);

int
read_file_in_directory(const char *file_path, const char *dirname, void* read_buffer, size_t buffer_size);

static char 
option_names[7][128] = {"write file", "read file", "write directory", "read n files", "lock", "unlock", "remove"};

static inline void
print_action(void *e, FILE *stream)
{
    action_t *a = (action_t*)e;
    fprintf(stream, "TYPE: %s\nARGS: %s\nDIR: %s\nTIME: %ld\n",
            option_names[a->code], a->arguments, a->directory, a->wait_time);
}

static inline void
free_action(void *e)
{
    action_t *a = (action_t*)e;
    if (a->directory) free(a->directory);
    if (a->arguments) free(a->arguments);
    free(a);
}
#endif