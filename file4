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
    WRITE_DIRECTORY     = 105,
    READ_FILE           = 106,
    READ_N_FILES        = 107,
    DELETE_FILE         = 108,   
    LOCK_FILE           = 109,
    UNLOCK_FILE         = 110,    

} request_code;

/**
 * Codes of the different statuses
 * that can be encountered
 */
typedef enum {
    
    ACCEPTED            = 0,
    SUCCESS             = 1,
    INTERNAL_ERROR      = 2,
    BAD_REQUEST         = 3,
    NOT_FOUND           = 4,
    UNAUTHORIZED        = 5,
    MISSING_BODY        = 6,
    
} response_code;

/**
 * The type of a request
 */
typedef struct _request_t {

    /* Request identifier */
    request_code    type;
    /* File on which the request is performed */
    char            resource_path[MAX_PATH];
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
    /* Size of the response body */
    size_t          body_size;
    /* Body of the response (Nullable field) */
    void*           body;

} response_t;


/** 
 * Messages corresponding to a
 * certain response status
 */
static char status_message[7][128] = {
    "Connection accepted",
    "Operation successfull",
    "Internal server error",
    "Bad request",
    "Entity not found",
    "Unauthorized access",
    "Missing message body"

};


/************** Request Handling Functions **************/


/**
 * Creates a new requests, returns the request on success, NULL on failure,
 * errno is set.
 */
request_t*
new_request(request_code type, char *resource_path, size_t body_size, void* body);

/**
 * Sends a request on socket associated with conn_fd, returns 0 on success,
 * -1 on failure, errno is set.
 */
int
send_request(int conn_fd, request_code type, char *resource_path, size_t body_size, void* body);

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
send_response(int conn_fd, response_code status, char *status_phrase, size_t body_size, void* body);

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
