#include "fileserver_api.h"

int
writeFile(const char* pathname, const char* dirname){
    
    if ( pathname == NULL ) {
        errno = EINVAL;
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        return -1;
    }

    FILE *file_ptr;

    file_ptr = fopen(pathname, "rb");
    if ( file_ptr == NULL ) {
        errno = EIO;
        return -1;
    }

    fseek(file_ptr, 0, SEEK_END);
    size_t file_size = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);
    if ( file_size == -1 ) {
        fclose(file_ptr);
        errno = EIO;
        return -1;
    }
        
    void* file_data;
    file_data = malloc(file_size);
    if ( file_data == NULL ) {
        fclose(file_ptr);
        errno = ENOMEM;
        return -1;
    }

    if ( fread(file_data, 1, file_size, file_ptr) < file_size ) {
        if ( ferror(file_ptr) ) {
            fclose(file_ptr);
            if ( file_data != NULL ) free(file_data);
            errno = EIO;
            return -1;
        }
    }

    fclose(file_ptr);

    int result;
    errno = 0;
    
    if ( send_request(socket_fd, WRITE_FILE, pathname, file_size, file_data) != 0) return -1;

    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    switch ( response->status ) {

        case INTERNAL_ERROR: {
            errno = ECONNABORTED;
            result = -1;
            break;
        }

        case MISSING_BODY: {
            errno = EINVAL;
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
    if (file_data) free(file_data);
    return result;
}