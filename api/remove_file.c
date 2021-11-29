#include "fileserver_api.h"

int 
removeFile(const char* pathname)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        return -1;
    }
    
    int result;
    errno = 0;

    if ( send_request(socket_fd, REMOVE_FILE, pathname, 0, NULL) != 0 ) return -1;

    response_t *response = recv_response(socket_fd);
    if ( response == NULL) return -1;

    switch ( response->status ) {

        case INTERNAL_ERROR: {
            errno = ECONNABORTED;
            result = -1;
            break;
        }

        case NOT_FOUND: {
            errno = ENOENT;
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            errno = EPERM;
            result = -1;
            break;
        }

        case SUCCESS: {
            result = 0;
            break;
        }

        default: {
            errno = EPROTONOSUPPORT;
            result = -1;
            break;
        }
    }
    
    if (response) free_response(response);

    return result;
}
