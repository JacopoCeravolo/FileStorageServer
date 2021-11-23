#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#define MAX_ARG_LENGTH  128

#include <stdio.h>
#include <stdlib.h>
#include "utils/linked_list.h"
#include "utils/utilities.h"

typedef enum {
    WRITE,
    READ,
    WRITE_DIR,
    READ_N,
    LOCK,
    UNLOCK,
    REMOVE
} option_code;

typedef struct _option_t {
    option_code type;
    char        arguments[MAX_ARG_LENGTH];
} option_t;


extern bool VERBOSE;
extern char socket_name[MAX_PATH];

int
parse_options(int argc,  char * const argv[], const char *options_string, list_t *parsed_options);


static void
print_option(void *e, FILE *f)
{
    option_t *o = (option_t*)e;

    char option_names[7][128] = {"write file", "read file", "write directory", "read n files", "lock", "unlock", "remove"};

    printf("%s -> %s\n", option_names[o->type], o->arguments);
}

static void
free_option(void *e)
{
    option_t *o = (option_t*)e;
    free(o);
}

#endif