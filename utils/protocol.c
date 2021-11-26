#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "protocol.h"
#include "utilities.h"


/************** Request Handling Functions **************/


request_t*
new_request(request_code type, char *resource_path, size_t body_size, void* body)
{
    request_t *request;
    request = (request_t*)calloc(1, sizeof(request_t));
    if (request == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    request->type = type;
    strncpy(request->resource_path, resource_path, MAX_PATH);

    if (body_size != 0) {
        request->body_size = body_size;
        request->body = calloc(1, body_size);
        if (request->body == NULL) {
            free(request);
            errno = ENOMEM;
            return NULL;
        }
        memcpy(request->body, body, body_size);
    } else {
        request->body_size = 0;
        request->body = NULL;
    }

    return request;
}

int
send_request(int conn_fd, request_code type, const char *resource_path, size_t body_size, void* body)
{
    if (conn_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    int result;
    if (write(conn_fd, (void*)&type, sizeof(response_code)) == -1) return -1;
    if (write(conn_fd, (void*)resource_path, sizeof(char) * MAX_PATH) == -1) return -1;
    if (write(conn_fd, (void*)&body_size, sizeof(size_t)) == -1) return -1;

    if (body_size != 0) {
        if (write(conn_fd, body, body_size) == -1) return -1;
    }
    return 0;
}

request_t*
recv_request(int conn_fd)
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

    if (read(conn_fd, (void*)&request->type, sizeof(response_code)) == -1) return NULL;
    if (read(conn_fd, (void*)request->resource_path, sizeof(char) * MAX_PATH) == -1) return NULL;           
    if (read(conn_fd, (void*)&request->body_size, sizeof(size_t)) == -1) return NULL;

    if (request->body_size != 0) {
        request->body = calloc(1, request->body_size);
        if (request->body == NULL) {
            free(request);
            errno = ENOMEM;
            return NULL;
        }

        if (read(conn_fd, request->body, request->body_size) == -1) return NULL;
    }

    return request;
}

void
free_request(request_t *request)
{
    if (request->body) free(request->body);
    if (request) free(request);
}


/************** Response Handling Functions **************/


response_t*
new_response(response_code status, char *status_phrase, size_t body_size, void* body)
{
    response_t *response;
    response = (response_t*)calloc(1, sizeof(response_t));
    if (response == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    response->status = status;
    strncpy(response->status_phrase, status_phrase, MAX_PATH);

    if (body_size != 0) {
        response->body_size = body_size;
        response->body = calloc(1, body_size);
        if (response->body == NULL) {
            free(response);
            errno = ENOMEM;
            return NULL;
        }
        memcpy(response->body, body, body_size);
    } else {
        response->body_size = 0;
        response->body = NULL;
    }

    return response;
}

int
send_response(int conn_fd, response_code status, const char *status_phrase, char *file_path, size_t body_size, void* body)
{
    if (conn_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    int result;
    if (write(conn_fd, (void*)&status, sizeof(response_code)) == -1) return -1;
    if (write(conn_fd, (void*)status_phrase, sizeof(char) * MAX_PATH) == -1) return -1;
    if (write(conn_fd, (void*)file_path, sizeof(char) * MAX_PATH) == -1) return -1;
    if (write(conn_fd, (void*)&body_size, sizeof(size_t)) == -1) return -1;

    if (body_size != 0) {
        if (write(conn_fd, body, body_size) == -1) return -1;
    }
    return 0;
}

response_t*
recv_response(int conn_fd)
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
    if (read(conn_fd, (void*)response->file_path, sizeof(char) * MAX_PATH) == -1) return NULL;
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
    if (response->body) {
        
        free(response->body); }
    if (response) {
        free(response);
         }
}

