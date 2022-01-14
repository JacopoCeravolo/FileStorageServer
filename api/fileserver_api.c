#include "fileserver_api.h"

#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "utils/protocol.h"
#include "utils/linked_list.h"
#include "utils/utilities.h"


long        socket_fd  = -1;        // socket file descriptor
char        *socket_path;           // saved socket path
list_t      *opened_files;          // list of currently opened files
char        result_buffer[2048];    // last request verbose result

#define set_errno_save_result(err, opt_type, path, n_bytes) { \
        sprintf(result_buffer, "%-15s %-60s %-10ld %-20s", opt_type, path, n_bytes, strerror(err)); \
        errno = err; }

#define save_request_result(opt_type, path, n_bytes, msg) { \
        sprintf(result_buffer, "%-15s %-60s %-10ld %-20s", opt_type, path, n_bytes, msg); }

void display_request_result() { printf("%s\n", result_buffer); }     
        
int 
openConnection(const char *sockname, int msec, const struct timespec abstime)
{
    // Validation of parameters
    if ( sockname == NULL || msec < 0 ) {
       errno = EINVAL;
       return -1;
    }

    // Verify if socket is already connected
    if ( socket_fd != -1 ) {
        errno = EISCONN;
        return -1;
    }    

    int tmp_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if ( tmp_socket_fd < 0 ) return -1;

    // Setting of sockaddr_un fields
    struct sockaddr_un serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, sockname);
    socket_fd = tmp_socket_fd;

    // Converting msec to timespec 
    struct timespec wait_time;
    wait_time.tv_sec = msec / 1000;
    msec = msec % 1000;
    wait_time.tv_nsec = msec * 1000;

    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    // Attemepts to open a connection
    int result;
    while ( (result = connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1 )
            && current_time.tv_sec < abstime.tv_sec ) {
        
        if ( nanosleep(&wait_time, NULL) == -1 ) {
            socket_fd = -1;
            return -1;
        }
        if ( clock_gettime(CLOCK_REALTIME, &current_time) == -1 ) {
            socket_fd = -1;
            return -1;        
        }
    }

    // Time has expired and connect failed
    if ( result < 0 ) { 
        set_errno_save_result(ECONNREFUSED, "openConnection", sockname, 0);
        return -1; 
    }

    // Sending handshake
    if ( send_request(socket_fd, OPEN_CONNECTION, 0, NULL, 0, NULL) != 0 ) return -1;

    // Receiving handshake and checking result
    response_t *handshake = recv_response(socket_fd);
    if ( handshake == NULL ) return -1;
    
    switch ( handshake->status ) {

        case SUCCESS: {
            // if successfull creates strcuture where to store opened files id
            opened_files = list_create(string_compare, free_string, NULL);
            if ( opened_files == NULL ) { result = -1; break; }

            socket_path = calloc(1, strlen(sockname) + 1);
            strcpy(socket_path, sockname);
            result = 0;
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "openConnection", sockname, 0);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "openConnection", sockname, 0);
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
    // Validation of parameters
    if ( strcmp(sockname, socket_path) != 0 ) {
        set_errno_save_result(EINVAL, "closeConnection", sockname, 0);
        return -1;
    }
    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "closeConnection", sockname, 0);
        return -1;
    }

    // Iterate over list of openend files
    while (!list_is_empty(opened_files)) {
        
        char *pathname = (char*)list_remove_head(opened_files);

        closeFile(pathname); // continue even if it fails

        free(pathname);
    }
    
    if ( send_request(socket_fd, CLOSE_CONNECTION, 0, NULL, 0, NULL) != 0 ) {
        close(socket_fd);
        list_destroy(opened_files);
        return -1;
    }
    
    close(socket_fd);
    list_destroy(opened_files);

    return 0;
}

int
openFile(const char* pathname, int flags)
{
    // Validation of parameters
    if ( pathname == NULL ) {
        set_errno_save_result(EINVAL, "openFile", pathname, 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "openFile", pathname, 0);
        return -1;
    }

    // Sending open file request
    if ( send_request(socket_fd, OPEN_FILE, strlen(pathname) +1, pathname, sizeof(int), &flags) != 0 ) return -1;

    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    int result = 0;
    switch ( response->status ) {

        case SUCCESS: {
            // If successfull add file name to list of opened files
            char *file_name = malloc(strlen(pathname) + 1);
            strcpy(file_name, pathname);
            if ( list_insert_tail(opened_files, file_name) != 0 ) return -1;
            
            save_request_result("openFile", pathname, 0, response->status_phrase);
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "openFile", pathname, 0);
            result = -1;
            break;
        }

        case NOT_FOUND: {
            set_errno_save_result(ENOENT, "openFile", pathname, 0);
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            set_errno_save_result(EPERM, "openFile", pathname, 0);
            result = -1;
            break;
        }

        case FILE_EXISTS: {
            set_errno_save_result(EEXIST, "openFile", pathname, 0);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "openFile", pathname, 0);
            result = -1;
            break;
        }
    }
    
    if (response) free_response(response);
    return result;
}

int 
readFile(const char* pathname, void** buf, size_t* size)
{
    // Validation of parameters
    if ( pathname == NULL ) {
        set_errno_save_result(EINVAL, "readFile", pathname, 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "readFile", pathname, 0);
        return -1;
    }

    // Generating absolute path
    char *absolute_path = realpath(pathname, NULL);

    // Checks if file has already been opened
    if ( list_find(opened_files, absolute_path ) == -1 ) {
        // File is not opened, perform openFile request
        if ( openFile(absolute_path, O_NOFLAG) != 0 ) return -1;
    }
    
    // Sending read file request
    if ( send_request(socket_fd, READ_FILE, strlen(absolute_path) + 1, absolute_path, 0, NULL) != 0 ) return -1;

    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    int result = 0;
    switch ( response->status ) {

        case SUCCESS: {

            // If successfull copies response body into reading buffer
            void* tmp_buf;
            tmp_buf = malloc(response->body_size);
            if ( tmp_buf == NULL ) {
                set_errno_save_result(ENOMEM, "readFile", absolute_path, 0);
                result = -1;
                break;
            }

            memcpy(tmp_buf, response->body, response->body_size);
            *buf = tmp_buf;
            *size = response->body_size;
            save_request_result("readFile", absolute_path, *size, response->status_phrase);
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "readFile", absolute_path, 0);
            result = -1;
            break;
        }

        case NOT_FOUND: {
            set_errno_save_result(ENOENT, "readFile", absolute_path, 0);
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            set_errno_save_result(EPERM, "readFile", absolute_path, 0);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "readFile", absolute_path, 0);
            result = -1;
            break;
        }
    }

    if (response) free_response(response);
    free(absolute_path);
    return result;
}

int 
readNFiles(int N, const char* dirname)
{
    // Validation of parameters
    if ( N < 0 ) {
        set_errno_save_result(EINVAL, "readNFiles", "", 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "readNFile", "", 0);
        return -1;
    }

    // Sending read n files request
    if ( send_request(socket_fd, READ_N_FILES, 0, "", sizeof(int), (void*)&N) != 0 ) return -1;
    
    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    int result = 0;
    switch (response->status) {

        case SUCCESS: {

            // Creates directory if not null
            if (dirname != NULL) {
                if (mkdir_p(dirname) == -1) {
                    result = -1;
                    break;
                }
            }

            // Getting number of files read
            int how_many = *(int*)response->body;

            // Receiving files
            int total_size_read = 0;
            while ( (how_many--) > 0) {
                
                response_t *received_file = recv_response(socket_fd);
                if ( received_file == NULL ) break; // if recv fails keep on going to receive next file

                total_size_read += received_file->body_size;

                // Writes received file in directory
                if (dirname != NULL) {
                    write_file_in_directory(dirname, received_file->file_path, received_file->body_size, received_file->body);
                    
                }

                free_response(received_file);
            }

            save_request_result("readNFiles", "", total_size_read, response->status_phrase);
            result = 0;
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "readNFile", "", 0);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "readNFile", "", 0);
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
    // Validation of parameters
    if ( pathname == NULL ) {
        set_errno_save_result(EINVAL, "writeFile", pathname, 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "writeFile", pathname, 0);
        return -1;
    }

    // Opening file from disk
    char *absolute_path = realpath(pathname, NULL);
    FILE *file_ptr = fopen(absolute_path, "rb");
    if ( file_ptr == NULL ) {
        set_errno_save_result(EIO, "writeFile", absolute_path, 0);
        return -1;
    }

    // Retreiving file size
    struct stat st;
    size_t file_size;
    if( stat(absolute_path, &st) == 0 ){
        file_size = st.st_size;
    } else {
        fclose(file_ptr);
        set_errno_save_result(EIO, "writeFile", absolute_path, 0);
        return -1;
    }
        
    // Reading file contents
    void* file_data;
    file_data = malloc(file_size);
    if ( file_data == NULL ) {
        fclose(file_ptr);
        set_errno_save_result(ENOMEM, "writeFile", absolute_path, 0);
        return -1;
    }

    if ( fread(file_data, 1, file_size, file_ptr) < file_size ) {
        if ( ferror(file_ptr) ) {
            fclose(file_ptr);
            if ( file_data != NULL ) free(file_data);
            set_errno_save_result(EIO, "writeFile", absolute_path, 0);
            return -1;
        }
    }

    // Checks if file has already been opened
    if ( list_find(opened_files, absolute_path ) == -1 ) {
        // File is not opened, perform openFile request
        if ( openFile(absolute_path, O_CREATE|O_LOCK ) != 0 ) return -1;
    }

    // Sending write file request
    if ( send_request(socket_fd, WRITE_FILE, strlen(absolute_path) + 1, absolute_path, file_size, file_data) != 0) return -1;

    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    int result = 0;
    switch ( response->status ) {

        case SUCCESS: {
            save_request_result("writeFile", absolute_path, file_size, response->status_phrase);
            break;
        }

        case FILES_EXPELLED: {

            // Creates directory if not null
            if ( dirname != NULL ) {
                if ( mkdir_p(dirname) == -1 ) {
                    result = -1;
                    break;
                }
            }

            // Getting number of files expelled
            int how_many = *(int*)response->body;

            // Receiving expelled files
            while ( (how_many--) > 0) {

                response_t *received_file = recv_response(socket_fd);
                if ( received_file == NULL ) break; // if recv fails keep on going to receive next files

                // Writes received file in directory
                if ( dirname != NULL ) {
                    write_file_in_directory(dirname, received_file->file_path, received_file->body_size, received_file->body);
                }

                // Since file was expelled, it should be remove from opened files if present
                list_remove_element(opened_files, received_file->file_path);

                save_request_result("writeFile", received_file->file_path, received_file->body_size, received_file->status_phrase);
                display_request_result(); // should do it only when verbose

                free_response(received_file);
            }

            // Receiving final response
            response_t *final_response = recv_response(socket_fd);

            switch ( final_response->status ) {
                case SUCCESS: save_request_result("writeFile", absolute_path, file_size, final_response->status_phrase); break;
                case INTERNAL_ERROR: set_errno_save_result(ECONNABORTED, "writeFile", absolute_path, file_size); result = -1; break;
                case UNAUTHORIZED: set_errno_save_result(EPERM, "writeFile", absolute_path, file_size); result = -1; break;
                default: set_errno_save_result(EPROTONOSUPPORT, "writeFile", absolute_path, file_size); result = -1; break;
            }
            
            if (final_response) free_response(final_response);
            
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "writeFile", absolute_path, file_size);
            result = -1;
            break;
        }

        case MISSING_BODY: {
            set_errno_save_result(EINVAL, "writeFile", absolute_path, file_size);
            result = -1;
            break;
        }

        case NOT_FOUND: {
            set_errno_save_result(ENOENT, "writeFile", absolute_path, file_size);
            result = -1;
            break;
        }

        case FILE_EXISTS: {
            set_errno_save_result(EEXIST, "writeFile", absolute_path, file_size);
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            set_errno_save_result(EPERM, "writeFile", absolute_path, file_size);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "writeFile", absolute_path, file_size);
            result = -1;
            break;
        }
    }
    
    fclose(file_ptr);
    if (response) free_response(response);
    if (file_data) free(file_data);
    free(absolute_path);
    return result;
}

int 
appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
{
    // Validation of parameters
    if ( pathname == NULL || buf == NULL || size < 0 ) {
        set_errno_save_result(EINVAL, "appendToFile", pathname, 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "appendToFile", pathname, 0);
        return -1;
    }

    // Generating absolute path
    char *absolute_path = realpath(pathname, NULL);
    
    // Checks if file has already been opened
    if ( list_find(opened_files, absolute_path ) == -1) {
        // File is not opened, perform openFile request
        if ( openFile(absolute_path, O_LOCK ) != 0 ) return -1;
    }

    // Sending append to file request
    if ( send_request(socket_fd, APPEND_TO_FILE, strlen(absolute_path) + 1, absolute_path, size, buf) != 0) return -1;

    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL ) return -1;

    int result = 0;
    switch ( response->status ) {

        case SUCCESS: {
            save_request_result("appendToFile", absolute_path, size, response->status_phrase);
            break;
        }

        case FILES_EXPELLED: {

            // Creates directory if not null
            if ( dirname != NULL ) {
                if ( mkdir_p(dirname) == -1 ) {
                    result = -1;
                    break;
                }
            }

            // Getting number of files expelled
            int how_many = *(int*)response->body;

            // Receiving expelled files
            while ( (how_many--) > 0) {

                response_t *received_file = recv_response(socket_fd);
                if ( received_file == NULL ) break; // if recv fails keep on going to receive next files

                // Writes received file in directory
                if ( dirname != NULL ) {
                    write_file_in_directory(dirname, received_file->file_path, received_file->body_size, received_file->body);
                }

                // Since file was expelled, it should be remove from opened files if present
                list_remove_element(opened_files, received_file->file_path);

                save_request_result("appendToFile", received_file->file_path, received_file->body_size, received_file->status_phrase);
                display_request_result(); // should do it only when verbose

                free_response(received_file);
            }

            // Receiving final response
            response_t *final_response = recv_response(socket_fd);

            switch ( final_response->status ) {
                case SUCCESS: save_request_result("appendToFile", absolute_path, size, final_response->status_phrase); break;
                case INTERNAL_ERROR: set_errno_save_result(ECONNABORTED, "appendToFile", absolute_path, size); result = -1; break;
                default: set_errno_save_result(EPROTONOSUPPORT, "writeFile", absolute_path, size); result = -1; break;
            }
            
            if (final_response) free_response(final_response);
            
            break;
        }
        
        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "appendToFile", absolute_path, size);
            result = -1;
            break;
        }

        case MISSING_BODY: {
            set_errno_save_result(EINVAL, "appendToFile", absolute_path, 0);
            result = -1;
            break;
        }

        case NOT_FOUND: {
            set_errno_save_result(ENOENT, "appendToFile", absolute_path, size);
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            set_errno_save_result(EPERM, "appendToFile", absolute_path, size);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "appendToFile", absolute_path, size);
            result = -1;
            break;
        }
    }
    
    if (response) free_response(response);
    free(absolute_path);
    return result;
}

int 
lockFile(const char* pathname)
{
    // Validation of parameters
    if ( pathname == NULL ) {
        set_errno_save_result(EINVAL, "lockFile", pathname, 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "lockFile", pathname, 0);
        return -1;
    }

    // Generating absolute path
    char *absolute_path = realpath(pathname, NULL);
    
    // Checks if file has already been opened
    if (list_find(opened_files, absolute_path) == -1 ) {
        // File is not opened, perform openFile request
        if (openFile(absolute_path, O_NOFLAG) != 0 ) return -1;
    }
    
    // Sending lock file request
    if ( send_request(socket_fd, LOCK_FILE, strlen(absolute_path) + 1, absolute_path, 0, NULL) != 0 ) return -1;

    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL) return -1;

    int result = 0;
    switch ( response->status ) {

        case SUCCESS: {
            save_request_result("lockFile", absolute_path, 0, response->status_phrase);
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "lockFile", absolute_path, 0);
            result = -1;
            break;
        }

        case NOT_FOUND: {
            set_errno_save_result(ENOENT, "lockFile", absolute_path, 0);
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            set_errno_save_result(EPERM, "lockFile", absolute_path, 0);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "lockFile", absolute_path, 0);
            result = -1;
            break;
        }
    }
    
    if (response) free_response(response);
    free(absolute_path);
    return result;
}

int 
unlockFile(const char* pathname)
{
    // Validation of parameters
    if ( pathname == NULL ) {
        set_errno_save_result(EINVAL, "unlockFile", pathname, 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "unlockFile", pathname, 0);
        return -1;
    }

    // Generating absolute path
    char *absolute_path = realpath(pathname, NULL);

    // Fails if file has not already been opened
    if (list_find(opened_files, absolute_path) == -1 ) {
        set_errno_save_result(EPERM, "unlockFile", absolute_path, 0 );
        return -1;
    }

    // Sending unlock file request
    if ( send_request(socket_fd, UNLOCK_FILE, strlen(absolute_path) + 1, absolute_path, 0, NULL) != 0 ) return -1;

    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL) return -1;

    int result = 0;
    switch ( response->status ) {

        case SUCCESS: {
            save_request_result("unlockFile", absolute_path, 0, response->status_phrase);
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "unlockFile", absolute_path, 0);
            result = -1;
            break;
        }

        case NOT_FOUND: {
            set_errno_save_result(ENOENT, "unlockFile", absolute_path, 0);
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            set_errno_save_result(EPERM, "unlockFile", absolute_path, 0);
            result = -1;
            break;
        }

        case BAD_REQUEST: {
            set_errno_save_result(EBADMSG, "unlockFile", absolute_path, 0);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "unlockFile", absolute_path, 0);
            result = -1;
            break;
        }

    }
    
    if (response) free_response(response);
    free(absolute_path);
    return result;
}
 
int 
removeFile(const char* pathname)
{
    // Validation of parameters
    if ( pathname == NULL ) {
        set_errno_save_result(EINVAL, "unlockFile", pathname, 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "unlockFile", pathname, 0);
        return -1;
    }

    // Generating absolute path
    char *absolute_path = realpath(pathname, NULL);

    // Checks if file has already been opened
    if ( list_find(opened_files, absolute_path) == -1 ) {
        // File is not opened, perform openFile request
        if ( openFile(absolute_path, O_LOCK) != 0 ) return -1;
    }
    
    // Sending remove file request
    if ( send_request(socket_fd, REMOVE_FILE, strlen(absolute_path) + 1, absolute_path, 0, NULL) != 0 ) return -1;

    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL) return -1;

    int result = 0;
    switch ( response->status ) {

        case SUCCESS: {
            // If successfull, removes file from opened files list
            if ( list_remove_element(opened_files, absolute_path) != 0 ) {
                perror("list_remove_element()");
            }
            save_request_result("removeFile", absolute_path, 0, response->status_phrase);
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "removeFile", absolute_path, 0);
            result = -1;
            break;
        }

        case NOT_FOUND: {
            set_errno_save_result(ENOENT, "removeFile", absolute_path, 0);
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            set_errno_save_result(EPERM, "removeFile", absolute_path, 0);
            result = -1;
            break;
        }

        case BAD_REQUEST: {
            set_errno_save_result(EBADMSG, "removeFile", absolute_path, 0);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "removeFile", absolute_path, 0);
            result = -1;
            break;
        }
    }
    
    if (response) free_response(response);
    free(absolute_path);
    return result;
}

int 
closeFile(const char* pathname)
{
    // Validation of parameters
    if ( pathname == NULL ) {
        set_errno_save_result(EINVAL, "closeFile", pathname, 0);
        return -1;
    }

    // Verify if connection is opened
    if ( socket_fd == -1 ) {
        set_errno_save_result(ENOTCONN, "closeFile", pathname, 0);
        return -1;
    }

    // Generating absolute path
    char *absolute_path = realpath(pathname, NULL);
    
    // Checks if file has been opened
    /* if ( list_find(opened_files, absolute_path) == -1 ) {
        set_errno_save_result(EPERM, "closeFile", absolute_path, 0);
        return -1;
    } */

    // Sending close file request
    if ( send_request(socket_fd, CLOSE_FILE, strlen(absolute_path) + 1, absolute_path, 0, NULL) != 0 ) return -1;

    // Receiving response and checking result
    response_t *response = recv_response(socket_fd);
    if ( response == NULL) return -1;

    int result = 0;
    switch ( response->status ) {

        case SUCCESS: {
            save_request_result("removeFile", absolute_path, 0, response->status_phrase);
            break;
        }

        case INTERNAL_ERROR: {
            set_errno_save_result(ECONNABORTED, "closeFile", absolute_path, 0);
            result = -1;
            break;
        }

        case NOT_FOUND: {
            set_errno_save_result(ENOENT, "closeFile", absolute_path, 0);
            result = -1;
            break;
        }

        case UNAUTHORIZED: {
            set_errno_save_result(EPERM, "closeFile", absolute_path, 0);
            result = -1;
            break;
        }

        default: {
            set_errno_save_result(EPROTONOSUPPORT, "closeFile", absolute_path, 0);
            result = -1;
            break;
        }
    }
    
    if (response) free_response(response);
    free(absolute_path);
    return result;
}

int
write_file_in_directory(const char *dirname, char *filename, size_t size, void* contents)
{
    // Strips basename from filename
    char *base_name = basename(filename); 

    // Constructing full path
    char *full_path = malloc(strlen(dirname) + strlen(base_name) + 1);
    sprintf(full_path, "%s/%s", dirname, base_name);
    
    // Writing file
    FILE *fp = fopen(full_path, "wb");
    if ( fp == NULL ) return -1;
    if ( fwrite(contents, 1, size, fp) == -1 ) return -1;

    return 0;   
}