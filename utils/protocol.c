#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "protocol.h"
#include "utilities.h"

/** 
 * Messages corresponding to a
 * certain response status
 */
static char status_message[10][128] = {
    "Operation successfull",
    "Connection accepted",
    "Internal server error",
    "Bad request",
    "Entity not found",
    "Unauthorized access",
    "Missing message body",
    "File exceeds maximum space",
    "File expelled",
    "File already exists"
};

const char*
get_status_message(response_code code)
{
    if (code < 0 || code > 9) return NULL;
    return status_message[code];
}

/************** Request Handling Functions **************/

int
send_request(long conn_fd, request_code type, size_t path_len, const char *resource_path, size_t body_size, void* body)
{
    if (conn_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    int result = 0;
    
    // Writes type
    if (write(conn_fd, (void*)&type, sizeof(response_code)) == -1) return -1;

    // Writes file path length
    if (write(conn_fd, (void*)&path_len, sizeof(size_t)) == -1) return -1;

    // Writes file path
    if (path_len != 0) {
        if (write(conn_fd, resource_path, sizeof(char) * path_len) == -1) return -1;
    }

    // Writes body size
    if (write(conn_fd, (void*)&body_size, sizeof(size_t)) == -1) return -1;

    // Writes body
    if (body_size != 0) {
        if (write(conn_fd, body, body_size) == -1) return -1;
    }

    return result;
}

request_t*
recv_request(long conn_fd)
{   
    if (conn_fd < 0) {
        errno = EINVAL;
        return NULL;
    }

    request_t *request;
    request = (request_t*)calloc(1, sizeof(request_t));
    if (request == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // Reads type
    if (read(conn_fd, (void*)&request->type, sizeof(response_code)) == -1) return NULL;
    
    // Read file path length
    if (read(conn_fd, (void*)&request->path_len, sizeof(size_t)) == -1) return NULL;
    
    // Allocates space for file path
    if (request->path_len != 0) {
        request->file_path = calloc(1, request->path_len);
        if (request->file_path == NULL) {
            free(request);
            errno = ENOMEM;
            return NULL;
        }

        // Reads file path
        if (read(conn_fd, (void*)request->file_path, sizeof(char) * request->path_len) == -1) return NULL; 
    }

    // Reads body size      
    if (read(conn_fd, (void*)&request->body_size, sizeof(size_t)) == -1) return NULL;

    // Allocates space for body
    if (request->body_size != 0) {
        request->body = calloc(1, request->body_size);
        if (request->body == NULL) {
            free(request);
            errno = ENOMEM;
            return NULL;
        }

        // Reads body
        if (read(conn_fd, request->body, request->body_size) == -1) return NULL;
    }

    return request;
}

void
free_request(request_t *request)
{
    if (request != NULL) {
        if (request->body) free(request->body);
        if (request->file_path) free(request->file_path);
        free(request);
    }
    
}


/************** Response Handling Functions **************/

int
send_response(long conn_fd, response_code status, const char *status_phrase, size_t path_len, char *file_path, size_t body_size, void* body)
{
    if (conn_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    int result = 0;
    if (write(conn_fd, (void*)&status, sizeof(response_code)) == -1) return -1;
    if (write(conn_fd, (void*)status_phrase, sizeof(char) * MAX_PATH) == -1) return -1;
    
    // Writes file path length
    if (write(conn_fd, (void*)&path_len, sizeof(size_t)) == -1) return -1;

    // Writes file path
    if (path_len != 0) {
        if (write(conn_fd, file_path, sizeof(char) * path_len) == -1) return -1;
    }
    if (write(conn_fd, (void*)&body_size, sizeof(size_t)) == -1) return -1;

    if (body_size != 0) {
        if (write(conn_fd, body, body_size) == -1) return -1;
    }
    return result;
}

response_t*
recv_response(long conn_fd)
{
    if (conn_fd < 0) {
        errno = EINVAL;
        return NULL;
    }

    response_t *response;
    response = (response_t*)calloc(1, sizeof(response_t));
    if (response == NULL) {
        errno = ENOMEM;
        return NULL;
    }


    if (read(conn_fd, (void*)&response->status, sizeof(response_code)) == -1) return NULL;
    if (read(conn_fd, (void*)response->status_phrase, sizeof(char) * MAX_PATH) == -1) return NULL;           
    
    // Read file path length
    if (read(conn_fd, (void*)&response->path_len, sizeof(size_t)) == -1) return NULL;
    
    // Allocates space for file path
    if (response->path_len != 0) {
        response->file_path = calloc(1, response->path_len);
        if (response->file_path == NULL) {
            free(response);
            errno = ENOMEM;
            return NULL;
        }

        // Reads file path
        if (read(conn_fd, (void*)response->file_path, sizeof(char) * response->path_len) == -1) return NULL; 
    }
    if (read(conn_fd, (void*)&response->body_size, sizeof(size_t)) == -1) return NULL;

    if (response->body_size != 0) {
        response->body = calloc(1, response->body_size);
        if (response->body == NULL) {
            free(response);
            errno = ENOMEM;
            return NULL;
        }

        if (read(conn_fd, response->body, response->body_size) == -1) return NULL;
    }

    return response;
}

void
free_response(response_t *response)
{
    if (response != NULL) {
        if (response->body) free(response->body);
        if (response->file_path) free(response->file_path);
        free(response);
    }
}

