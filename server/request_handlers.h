#ifndef REQUEST_HANDLERS_H
#define REQUEST_HANDLERS_H

#include "utils/protocol.h"
#include "utils/linked_list.h"

int
open_connection_handler(int client_fd);

int
close_connection_handler(int client_fd);

int
open_file_handler(int client_fd, request_t *request, list_t *expelled_files);

int
close_file_handler(int client_fd, request_t *request);

int
write_file_handler(int client_fd, request_t *request, list_t *expelled_files);

int
read_file_handler(int client_fd, request_t *request, void** read_buffer, size_t *size);

int
remove_file_handler(int client_fd, request_t *request);

int
lock_file_handler(int client_fd, request_t *request);

int
unlock_file_handler(int client_fd, request_t *request);

#endif