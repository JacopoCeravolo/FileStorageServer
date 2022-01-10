#include "fileserver_api.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdarg.h>
#include <libgen.h>
#include "unistd.h"

#include "utils/protocol.h"
#include "utils/linked_list.h"
#include "utils/hash_map.h"
#include "utils/utilities.h"

long socket_fd  = -1;
list_t *opened_files;
hash_map_t *opened_map;
char result_buffer[2048];

void
set_result_buffer(char *opt_type, char *filename, size_t n_bytes, char *result)
{
    sprintf(result_buffer, "%-10s %-65s %-10lu %-20s", opt_type, filename, n_bytes, result);
}

void
print_result()
{
    printf("%s\n", result_buffer);
}

int
write_file_in_directory(char *dirname, char *filename, size_t size, void* contents)
{
    //char *dir_path = realpath(dirname, NULL);
    char *base_name = basename(filename);
    char *full_path = malloc(strlen(dirname) + strlen(base_name) + 1);
    sprintf(full_path, "%s/%s", dirname, base_name);
    
    FILE *fp = fopen(full_path, "wb");

    fwrite(contents, 1, size, fp);
    return 0;
    // use real_path
    // free(dir_path);    
}

int 
openConnection(const char *sockname, int msec, const struct timespec abstime)
{
    if ( socket_fd != -1 ) {
        errno = EISCONN;
        return -1;
    }

    if ( sockname == NULL || msec < 0 ) {
       errno = EINVAL;
       return -1;
    }


    int result;
    struct sockaddr_un serveraddr;

    int tmp_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (tmp_socket_fd < 0) return -1;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, sockname);
    socket_fd = tmp_socket_fd;

    struct timespec wait_time;
    wait_time.tv_sec = msec / 1000;
    msec = msec % 1000;
    wait_time.tv_nsec = msec * 1000;

    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    while ( (result = connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
            && current_time.tv_sec < abstime.tv_sec ) {
        
        if( nanosleep(&wait_time, NULL) == -1){
            socket_fd = -1;
            return -1;
        }
        if( clock_gettime(CLOCK_REALTIME, &current_time) == -1){
            socket_fd = -1;
            return -1;        
        }
    }
    if (result < 0) return -1;

    if ( send_request(socket_fd, OPEN_CONNECTION, 0, NULL, 0, NULL) != 0 ) return -1;

    response_t *handshake = recv_response(socket_fd);
    if (handshake == NULL) return -1;
    
    switch ( handshake->status ) {

        case SUCCESS: {

            // if successfull creates strcuture where to store opened files id
            opened_files = list_create(string_compare, free, string_print);
            if (opened_files == NULL) return -1;

            result = 0;
            break;
        }

        case INTERNAL_ERROR: {
            errno = ECONNABORTED;
            result = -1;
            break;
        }

        default: {
            errno = EPROTONOSUPPORT;
            result = -1;
            break;
        }
    }

    if (handshake) free_response(handshake);
    return result;
}

int 
closeConnection(const char* sockname)
{
    int result = 0;

    char *pathname;

    while (!list_is_empty(opened_files)) {
        pathname = (char*)list_remove_head(opened_files);
        if (pathname == NULL) perror("list_remove_head()");

        if (closeFile(pathname) != 0) { // perror("closeFile()"); 
        }
    }
    
    send_request(socket_fd, CLOSE_CONNECTION, 0, NULL, 0, NULL);
    
    close(socket_fd);
    list_destroy(opened_files);
    return result;
}

int
openFile(const char* pathname, int flags)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        set_result_buffer("openFile", pathname, 0, strerror(errno));
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        set_result_buffer("openFile", pathname, 0, strerror(errno));
        return -1;
    }

    // SET SOME KIND OF FLAGS TO REMEMBER LAST OPERATION WAS OPEN

    int result;
    errno = 0;

    if ( send_request(socket_fd, OPEN_FILE, strlen(pathname) +1, pathname, sizeof(int), &flags) != 0 ) return -1;

    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    switch ( response->status ) {

        case SUCCESS: {

            // If successfull add file name to list of opened files
            char *file_name = strdup(pathname);
            if (list_insert_tail(opened_files, file_name) != 0) return -1;
            result = 0;
            break;
        }

        /* case FILES_EXPELLED: {

            set_result_buffer("openFile", response->file_path, response->body_size, response->status_phrase);
            print_result();

            // Receiving final response
            response_t *final_response = recv_response(socket_fd);
            if (final_response == NULL) {
                perror("Response is null");
                return -1;
            }

            switch (final_response->status) {
                case SUCCESS: result = 0;
                case INTERNAL_ERROR: errno = ECONNABORTED; result = -1;
                case NOT_FOUND: errno = ENOENT; result = -1;
                case FILE_EXISTS: errno = EEXIST; result = -1;
                default: errno = EPROTONOSUPPORT; result = -1;
            }

            set_result_buffer("openFile", final_response->file_path, final_response->body_size, final_response->status_phrase);

            if (response) free_response(response);
            if (final_response) free_response(final_response);
            return result;
        } */

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

        default: {
            errno = EPROTONOSUPPORT;
            result = -1;
            break;
        }
    }

    set_result_buffer("openFile", response->file_path, response->body_size, response->status_phrase);
    
    if (response) free_response(response);
    return result;
}

int 
readFile(const char* pathname, void** buf, size_t* size)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        set_result_buffer("readFile", pathname, 0, strerror(errno));
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        set_result_buffer("readFile", pathname, 0, strerror(errno));
        return -1;
    }

    // Checks if file has already been opened
    if (list_find(opened_files, pathname) == -1) {

        //printf("readFile(): list_find failed, opening file\n");
        // File is not opened, perform openFile request
        if (openFile(pathname, O_NOFLAG) != 0) return -1;
    }
    
    int result;
    errno = 0;

    if ( send_request(socket_fd, READ_FILE, strlen(pathname) + 1, pathname, 0, NULL) != 0 ) return -1;

    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    switch ( response->status ) {

        case SUCCESS: {

            // If successfull copies response body into buffer
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

        default: {
            errno = EPROTONOSUPPORT;
            result = -1;
            break;
        }
    }

    set_result_buffer("readFile", response->file_path, response->body_size, response->status_phrase);

    if (response) free_response(response);
    return result;
}

int 
readNFiles(int N, const char* dirname)
{
    if ( N < 0 ) {
        errno = EINVAL;
        set_result_buffer("readNFiles", "", 0, strerror(errno));
        return -1;
    }

    int result;
    errno = 0;

    if ( send_request(socket_fd, READ_N_FILES, 0, "", sizeof(int), (void*)&N) != 0 ) return -1;

    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    switch (response->status) {

        case SUCCESS: {

            if (dirname != NULL) {
                if (mkdir_p(dirname) == -1) {
                    result = -1;
                    break;
                }
            }

            int how_many = *(int*)response->body;

            while ( (how_many--) > 0) {
                response_t *received_file = recv_response(socket_fd);
                if (received_file == NULL) break;

                // printf("RECEIVED: file [%s] of size %lu\n", received_file->file_path, received_file->body_size);

                if (dirname != NULL) {
                    if (write_file_in_directory(dirname, received_file->file_path, received_file->body_size, received_file->body) != 0) {
                        perror("write_file_in_directory");
                    }
                }

                free_response(received_file);
            }
            result = 0;
            break;
        }

        case INTERNAL_ERROR: {
            errno = ECONNABORTED;
            result = -1;
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

int
writeFile(const char* pathname, const char* dirname)
{
    
    if ( pathname == NULL ) {
        errno = EINVAL;
        set_result_buffer("writeFile", pathname, 0, strerror(errno));
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        set_result_buffer("writeFile", pathname, 0, strerror(errno));
        return -1;
    }

    FILE *file_ptr;

    file_ptr = fopen(pathname, "rb");
    if ( file_ptr == NULL ) {
        errno = EIO;
        set_result_buffer("writeFile", pathname, 0, strerror(errno));
        return -1;
    }

    struct stat st;
    size_t file_size;
    if(stat(pathname, &st) == 0){
        file_size = st.st_size;
    } else {
        fclose(file_ptr);
        errno = EIO;
        set_result_buffer("writeFile", pathname, 0, strerror(errno));
        return -1;
    }

    /* fseek(file_ptr, 0, SEEK_END);
    size_t file_size = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);
    if ( file_size == -1 ) {
        fclose(file_ptr);
        errno = EIO;
        return -1;
    } */
        
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
            set_result_buffer("writeFile", pathname, 0, strerror(errno));
            return -1;
        }
    }

    fclose(file_ptr);

    // Checks if file has already been opened
    if (list_find(opened_files, pathname) == -1) {

        //printf("writeFile(): list_find failed, opening file\n");
        // File is not opened, perform openFile request
        if (openFile(pathname, O_CREATE|O_LOCK) != 0) return -1;
    }

    int result;
    errno = 0;
    
    if ( send_request(socket_fd, WRITE_FILE, strlen(pathname) + 1, pathname, file_size, file_data) != 0) return -1;

    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    switch ( response->status ) {

        case SUCCESS: {
            result = 0;
            break;
        }

        case FILES_EXPELLED: {

            if (dirname != NULL) {
                if (mkdir_p(dirname) == -1) {
                    result = -1;
                    break;
                }
            }

            int how_many = *(int*)response->body;
            

            while ( (how_many--) > 0) {

                response_t *received_file = recv_response(socket_fd);
                if (received_file == NULL) {
                    //printf("response is null\n");
                    break;
                }

                if (received_file->status != FILES_EXPELLED) {
                    free_response(received_file);
                    errno = EPROTONOSUPPORT;
                    break;
                }

                // printf("EXPELLED: received [%s] of size %lu\n", received_file->file_path, received_file->body_size);
                set_result_buffer("writeFile", received_file->file_path, received_file->body_size, received_file->status_phrase);
                print_result();

                if (dirname != NULL) {
                    write_file_in_directory(dirname, received_file->file_path, received_file->body_size, received_file->body);
                }

                list_remove_element(opened_files, received_file->file_path);

                free_response(received_file);
            }

            // Receiving final response
            response_t *final_response = recv_response(socket_fd);

            if (final_response->status != SUCCESS) {
                result = -1;
                break;
            }

            set_result_buffer("writeFile", final_response->file_path, file_size, final_response->status_phrase);
            result = 0;
            
            if (final_response) free_response(final_response);
            if (response) free_response(response);
            return result;
        }

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

        default: {
            errno = EPROTONOSUPPORT;
            result = -1;
            break;
        }
    }

    set_result_buffer("writeFile", response->file_path, file_size, response->status_phrase);
    
    if (response) free_response(response);
    if (file_data) free(file_data);
    return result;
}

int 
appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
    return 0;
}

int 
lockFile(const char* pathname)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        set_result_buffer("lockFile", pathname, 0, strerror(errno));
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        set_result_buffer("lockFile", pathname, 0, strerror(errno));
        return -1;
    }
    
    // Checks if file has already been opened
    if (list_find(opened_files, pathname) == -1) {

        //printf("lockFile(): list_find failed, opening file\n");
        // File is not opened, perform openFile request
        if (openFile(pathname, O_NOFLAG) != 0) return -1;
    }

    int result;
    errno = 0;

    if ( send_request(socket_fd, LOCK_FILE, strlen(pathname) + 1, pathname, 0, NULL) != 0 ) return -1;

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

    set_result_buffer("lockFile", response->file_path, response->body_size, response->status_phrase);
    
    if (response) free_response(response);

    return result;
}

int 
unlockFile(const char* pathname)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        set_result_buffer("unlockFile", pathname, 0, strerror(errno));
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        set_result_buffer("unlockFile", pathname, 0, strerror(errno));
        return -1;
    }
    
    int result;
    errno = 0;

    // Checks if file has already been opened
    if (list_find(opened_files, pathname) == -1) {
        errno = EPERM;
        set_result_buffer("unlockFile", pathname, 0, strerror(errno));
        // File is not opened, failing
        //printf("unlockFile(): list_find failed\n");
        return -1;
    }

    if ( send_request(socket_fd, UNLOCK_FILE, strlen(pathname) + 1, pathname, 0, NULL) != 0 ) return -1;

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

        case BAD_REQUEST: {
            errno = EBADMSG;
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

    set_result_buffer("unlockFile", response->file_path, response->body_size, response->status_phrase);
    
    if (response) free_response(response);

    return result;
}
 
int 
removeFile(const char* pathname)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        set_result_buffer("removeFile", pathname, 0, strerror(errno));
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        set_result_buffer("removeFile", pathname, 0, strerror(errno));
        return -1;
    }

    // Checks if file has already been opened
    if (list_find(opened_files, pathname) == -1) {
        //printf("removeFile(): list_find failed, opening file\n");
        // File is not opened, perform openFile request
        if (openFile(pathname, O_LOCK) != 0) return -1;
    }
    
    int result;
    errno = 0;
    
    if ( send_request(socket_fd, REMOVE_FILE, strlen(pathname) + 1, pathname, 0, NULL) != 0 ) return -1;

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
            list_remove_element(opened_files, pathname);
            result = 0;
            break;
        }

        default: {
            errno = EPROTONOSUPPORT;
            result = -1;
            break;
        }
    }

    set_result_buffer("removeFile", response->file_path, response->body_size, response->status_phrase);
    
    if (response) free_response(response);

    return result;
}

int 
closeFile(const char* pathname)
{
    if ( pathname == NULL ) {
        errno = EINVAL;
        set_result_buffer("closeFile", pathname, 0, strerror(errno));
        return -1;
    }

    if ( socket_fd == -1 ) {
        errno = ENOTCONN;
        set_result_buffer("closeFile", pathname, 0, strerror(errno));
        return -1;
    }
    
    /* // Checks if file has already been opened
    if (list_find(opened_files, pathname) == -1) {
        //printf("closeFile(): list_find failed\n");
        // File is not opened, perform openFile request
        return -1;
    } */

    int result;
    errno = 0;

    size_t path_len = strlen(pathname) + 1;

    if ( send_request(socket_fd, CLOSE_FILE, path_len, pathname, 0, NULL) != 0 ) return -1;

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
