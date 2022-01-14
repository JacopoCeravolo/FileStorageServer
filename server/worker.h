#ifndef WORKER_H
#define WORKER_H

#include "server_config.h"

/**
 * Worker arguments
 */
typedef struct _worker_arg_t {
   int pipe_fd;
   int worker_id;
} worker_arg_t;

/**
 * Worker thread
 */
void*
worker_thread(void* args);

int
open_connection_handler(int worker_no, int client_fd);

int
close_connection_handler(int worker_no, int client_fd);

int
open_file_handler(int worker_no, int client_fd, request_t *request);

int
close_file_handler(int worker_no, int client_fd, request_t *request);

int
write_file_handler(int worker_no, int client_fd, request_t *request, list_t *expelled_files);

int
append_to_file_handler(int worker_no, int client_fd, request_t *request, list_t *expelled_files);

int
read_file_handler(int worker_no, int client_fd, request_t *request, void** read_buffer, size_t *size);

int
read_n_files_handler(int worker_no, int client_fd, request_t *request, list_t *files_list);

int
remove_file_handler(int worker_no, int client_fd, request_t *request);

int
lock_file_handler(int worker_no, int client_fd, request_t *request);

int
unlock_file_handler(int worker_no, int client_fd, request_t *request);

#endif