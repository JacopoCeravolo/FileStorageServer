#ifndef CLIENT_H
#define CLIENT_H

#include "utils/linked_list.h"

#define CLIENT_OPTIONS      "a:w:W:r:R:l:u:c:f:d:D:t:ph"
#define DEFAULT_SOCKET_PATH "/tmp/LSO_socket.sk"

/** 
 * Action types
 */
typedef enum {
    APPEND,
    WRITE,
    READ,
    WRITE_DIR,
    READ_N,
    LOCK,
    UNLOCK,
    REMOVE
} action_code;

/**
 * Action corresponding to a request
 */
typedef struct _action_t {
    action_code code;
    char        *arguments;
    char        *directory;
    long        wait_time;
} action_t;

/**
 * Reads a certain number of files from directory source_dir into files_list
 */
int 
get_files_from_directory(char *source_dir, int *how_many, list_t *files_list);

/**
 * Parse options from command line
 */
int
parse_options_in_list(int n_options, char * const option_vector[], list_t *action_list);

/**
 * Execute an action corresponding to a request
 */
int
execute_action(action_t *action);

/**
 * Deallocates an action
 */
void
free_action(void *e);

/**
 * Prints help message
 */
void
print_help_msg();

#endif