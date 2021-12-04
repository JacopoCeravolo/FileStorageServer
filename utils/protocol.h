#ifndef PROTOCOL_H
#define PROTOCOL_H


#include "utilities.h"

/************** Data Structures **************/


/**
 * Codes of the different requests
 * that can be performed
 */
typedef enum {
    
    OPEN_CONNECTION     = 100,
    CLOSE_CONNECTION    = 101,
    OPEN_FILE           = 102,
    CLOSE_FILE          = 103,
    WRITE_FILE          = 104,
    READ_FILE           = 105,
    READ_N_FILES        = 106,
    REMOVE_FILE         = 107,   
    LOCK_FILE           = 108,
    UNLOCK_FILE         = 109,    

} request_code;

/**
 * Codes of the different statuses
 * that can be encountered
 */
typedef enum {
    
    SUCCESS             = 0,
    ACCEPTED            = 1,
    INTERNAL_ERROR      = 2,
    BAD_REQUEST         = 3,
    NOT_FOUND           = 4,
    UNAUTHORIZED        = 5,
    MISSING_BODY        = 6,
    FILE_TOO_BIG        = 7,
    FILES_EXPELLED      = 8,
    FILE_EXISTS         = 9,
    
} response_code;

/**
 * The type of a request
 */
typedef struct _request_t {

    /* Request identifier */
    request_code    type;
    /* Path length */
    size_t          path_len;
    /* File on which the request is performed */
    char            *file_path;
    /* Size of the request body */
    size_t          body_size;
    /* Body of the request (Nullable field) */
    void*           body;

} request_t;

/**
 * The type of a response 
 */
typedef struct _response_t {

    /* Response status */
    response_code   status;
    /* Response status phrase */
    char            status_phrase[MAX_PATH]; // change size to MAX_STRING (?)
    /* File on which the operation was performed */
    char            file_path[MAX_PATH];
    /* Size of the response body */
    size_t          body_size;
    /* Body of the response (Nullable field) */
    void*           body;

} response_t;


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
    "Some files were expelled",
    "File already exists"
};


/************** Request Handling Functions **************/

/**
 * Sends a request on socket associated with conn_fd, returns 0 on success,
 * -1 on failure, errno is set.
 */
int
send_request(int conn_fd, request_code type, size_t path_len, const char *resource_path, size_t body_size, void* body);

/**
 * Receives a request on socket associated with conn_fd, return the request
 * on success, NULL on failure, errno is set.
 */
request_t*
recv_request(int conn_fd);

/**
 * Deallocates a requests and all of its components
 */
void
free_request(request_t *request);


/************** Response Handling Functions **************/


/**
 * Creates a new response, returns the response on success, NULL on failure,
 * errno is set.
 */
response_t*
new_response(response_code status, char *status_phrase, size_t body_size, void* body);

/**
 * Sends a response on socket associated with conn_fd, returns 0 on success,
 * -1 on failure, errno is set.
 */
int
send_response(int conn_fd, response_code status, const char *status_phrase, char *file_path, size_t body_size, void* body);

/**
 * Receives a response on socket associated with conn_fd, return the response
 * on success, NULL on failure, errno is set.
 */
response_t*
recv_response(int conn_fd);

/**
 * Deallocates a response and all of its components
 */
void
free_response(response_t *response);

#endif
