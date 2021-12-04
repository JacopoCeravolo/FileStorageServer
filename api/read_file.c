#include "fileserver_api.h"

int 
readFile(const char* pathname, void** buf, size_t* size)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        return -1;
    }

    if (openFile(pathname, O_NOFLAG) != 0) return -1;

    int result;
    errno = 0;

    if ( send_request(socket_fd, READ_FILE, strlen(pathname) + 1, pathname, 0, NULL) != 0 ) return -1;

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

        case UNAUTHORIZED: {
            errno = EPERM;
            result = -1;
            break;
        }

        case SUCCESS: {
            void* tmp_buf;
            tmp_buf = malloc(response->body_size);
            if ( tmp_buf == NULL ) {
                errno = ENOMEM;
                result = -1;
                break;
            }

            memcpy(tmp_buf, response->body, response->body_size);
            *buf = tmp_buf;
            *size = response->body_size;
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
