#ifndef WORKER_H
#define WORKER_H

#include "server_config.h"

typedef struct _worker_arg_t {
   concurrent_queue_t *requests;
   long *exit_signal;
   int pipe_fd;
   int worker_id;
} worker_arg_t;

void*
worker_thread(void* args);

int
open_connection_handlet(int worker_no, int client_fd);

int
close_connection_handlet(int worker_no, int client_fd);

int
open_file_handlet(int worker_no, int client_fd, request_t *request);

int
close_file_handlet(int worker_no, int client_fd, request_t *request);

int
write_file_handlet(int worker_no, int client_fd, request_t *request, list_t *expelled_files);

int
read_file_handlet(int worker_no, int client_fd, request_t *request, void** read_buffer, size_t *size);

int
read_n_files_handlet(int worker_no, int client_fd, request_t *request, list_t *files_list);

int
remove_file_handlet(int worker_no, int client_fd, request_t *request);

int
lock_file_handlet(int worker_no, int client_fd, request_t *request);

int
unlock_file_handlet(int worker_no, int client_fd, request_t *request);

#endif