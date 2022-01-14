#ifndef CLIENT_H
#define CLIENT_H

#include "utils/linked_list.h"

#define CLIENT_OPTIONS      "a:w:W:r:R:l:u:c:f:d:D:t:ph"
#define DEFAULT_SOCKET_PATH "/tmp/LSO_socket.sk"

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

typedef struct _action_t {
    action_code code;
    char        *arguments;
    char        *directory;
    long        wait_time;
} action_t;


int 
get_files_from_directory(char *source_dir, int *how_many, list_t *files_list);

int
parse_options_in_list(int n_options, char * const option_vector[], list_t *action_list);

int
execute_action(action_t *action);

void
free_action(void *e);

void
print_help_msg();

#endif