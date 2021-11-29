#include "fileserver_api.h"

int
openFile(const char* pathname, int flags)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        return -1;
    }

    // SET SOME KIND OF FLAGS TO REMEMBER LAST OPERATION WAS OPEN

    int result;
    errno = 0;

    if ( send_request(socket_fd, OPEN_FILE, pathname, sizeof(int), &flags) != 0 ) return -1;

    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

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

        case FILE_EXISTS: {
            errno = EEXIST;
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
