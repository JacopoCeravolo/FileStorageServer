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

#include "utils/utilities.h"
#include "utils/protocol.h"

int socket_fd;

void 
cleanup()
{
    close(socket_fd);
}

void
open_connection(int socket_fd)
{
    printf("\n> client opens connection\n");
    send_request(socket_fd, OPEN_CONNECTION, "", 0, NULL);
    response_t *r = recv_response(socket_fd);
    printf("> received: %d : %s\n", r->status, r->status_phrase);
    //free_response(r);
}

void
read_file(int socket_fd, char *file)
{
    printf("\n> client reading file %s\n", file);
    send_request(socket_fd, READ_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
    printf("> received: %d : %s : %lu\n", r->status, r->status_phrase, r->body_size);
    if (r->body_size != 0) printf("%s\n", (char*)r->body);
    //free_response(r);
}

void
remove_file(int socket_fd, char *file)
{
    printf("\n> client removing file %s\n", file);
    send_request(socket_fd, DELETE_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
    printf("> received: %d : %s : %lu\n", r->status, r->status_phrase, r->body_size);
    if (r->body_size != 0) printf("%s\n", (char*)r->body);
    //free_response(r);
}

void 
write_file(int socket_fd, char *file)
{
    printf("\n> client writing file %s\n", file);
    FILE *file_ptr;
        file_ptr = fopen(file, "rb");
        if (file_ptr == NULL) {
            errno = EIO;
            return;
        }

       
        fseek(file_ptr, 0, SEEK_END);
        size_t file_size = ftell(file_ptr);
        fseek(file_ptr, 0, SEEK_SET);
        if (file_size == -1) {
            fclose(file_ptr);
            return;
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
    
    send_request(socket_fd, WRITE_FILE, file, file_size, file_data);
    response_t *r = recv_response(socket_fd);
    printf("> received: %d : %s\n", r->status, r->status_phrase);

    if (r->status == FILES_EXPELLED) {
        int n_files = *(int*)r->body;
        printf("> %d files were expelled\n", n_files);

        while (n_files > 0) {
            response_t *r1 = recv_response(socket_fd);
            printf("> received: %d : %s : %lu\n", r1->status, r1->status_phrase, r1->body_size);
            printf("\n%s\n\n%s\n\n", r1->file_path, (char*)r1->body);
            n_files--;
        }
    }
    //free_response(r);
}

void
open_file(int socket_fd, char * file)
{
    printf("\n> client opening file %s\n", file);
    send_request(socket_fd, OPEN_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
    printf("> received: %d : %s\n", r->status, r->status_phrase);
    //free_response(r);
}

void
close_file(int socket_fd, char * file)
{
    printf("\n> client closing file %s\n", file);
    send_request(socket_fd, CLOSE_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
    printf("> received: %d : %s\n", r->status, r->status_phrase);
    //free_response(r);
}

void
close_connection(int socket_fd)
{
    printf("\n> client closing connection\n");
    send_request(socket_fd, CLOSE_CONNECTION, "", 0, NULL);
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


    open_connection(socket_fd);

    open_file(socket_fd, "file500");
    open_file(socket_fd, "file500_copy");
    // open_file(socket_fd, "file3");
    open_file(socket_fd, "full");
    /* open_file(socket_fd, "file3");
    open_file(socket_fd, "file4");
    open_file(socket_fd, "file5");
    open_file(socket_fd, "longfile"); */
    //sleep(20);


    write_file(socket_fd, "full");
    write_file(socket_fd, "file500");
    write_file(socket_fd, "file500_copy");
    // write_file(socket_fd, "file3");
    /* write_file(socket_fd, "file4");
    write_file(socket_fd, "file5");
    write_file(socket_fd, "longfile"); */

    //remove_file(socket_fd, "file2");
    //remove_file(socket_fd, "file3");
    //remove_file(socket_fd, "file5");

    //read_file(socket_fd, "file1");

    /* close_file(socket_fd, "file1");
    close_file(socket_fd, "file4"); */

    close_connection(socket_fd);

    /* do
    {
        int type;


       // WRITE

        type = READ_FILE;

        char resource[MAX_PATH] = "protocol.h";

        
        char *body;
        
        FILE *file_ptr;
        file_ptr = fopen(resource, "rb");
        if (file_ptr == NULL) {
            errno = EIO;
            return -1;
        }

       
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


        if (send_request(socket_fd, type, resource, file_size, file_data) == -1) {
            fprintf(stderr, "ERROR: %s", strerror(errno));
            return -1;
        }

        free(file_data);
        printf("> sent request\n");

        response_t *response;
        response = recv_response(socket_fd);
        if (response == NULL) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            free_response(response);
            return -1;
        }

        printf("> received: %d : %s : %lu \n", response->status, response->status_phrase, response->body_size);
        // free_response(response);

        // CLOSE 

        send_request(socket_fd, CLOSE_CONNECTION, "", 0, NULL); 

    } while (0); */
    

    

    return 0;
}
