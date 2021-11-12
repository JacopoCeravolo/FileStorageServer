#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

#include "utilities.h"
#include "protocol.h"

int socket_fd;

void 
cleanup()
{
    close(socket_fd);
}

int main(int argc, char const *argv[])
{
    atexit(cleanup);
    int result;
    struct sockaddr_un serveraddr;

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) return -1;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, DEFAULT_SOCKET_PATH);
    // strcpy(socket_name, sockname);

    result = connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); // Use SUN_LEN
    if (result < 0) return -1;

    int i = 0;
    do
    {
        int type = WRITE_FILE;

        char resource[MAX_PATH] = "file1";

        
        char *body;
        
        FILE *file_ptr;
        file_ptr = fopen(resource, "rb");
        if (file_ptr == NULL) {
            errno = EIO;
            return -1;
        }

        /* Get file size */
        fseek(file_ptr, 0, SEEK_END);
        long file_size = ftell(file_ptr);
        fseek(file_ptr, 0, SEEK_SET);
        if (file_size == -1) {
            fclose(file_ptr);
            return -1;
        }

        void* file_data;
        file_data = malloc(file_size);
        if (file_data == NULL) {
            errno = ENOMEM;
            fclose(file_ptr);
        }

        if (fread(file_data, 1, file_size, file_ptr) < file_size) {
            if (ferror(file_ptr)) {
                fclose(file_ptr);
                if (file_data != NULL) free(file_data);
            }
        }

        fclose(file_ptr);


        request_t *request;
        request = new_request(type, resource, file_size, file_data);
        if (send_request(socket_fd, request) == -1) {
            fprintf(stderr, "ERROR: %s", strerror(errno));
            free_request(request);
            return -1;
        }

        free_request(request);

        printf("> sent request\n");

        /* response_t *response;
        response = recv_response(socket_fd);
        if (response == NULL) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            free_response(response);
            return -1;
        }

        printf("> received: %d : %s : %lu : %s\n", response->status, response->status_phrase, response->body_size, (char*)response->body);
        free_response(response); */

    } while (0);
    
    return 0;
}
