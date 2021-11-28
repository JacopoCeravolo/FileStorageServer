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
#include <time.h>

#include "utils/utilities.h"
#include "utils/protocol.h"
#include "utils/logger.h"

#ifndef PRINT
#define PRINT 0
#endif


int socket_fd;

void 
cleanup()
{
    close(socket_fd);
}

void
open_connection(int socket_fd)
{
    if (PRINT) log_info("client opens connection\n");
    send_request(socket_fd, OPEN_CONNECTION, "", 0, NULL);
    response_t *r = recv_response(socket_fd);
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
    if (PRINT) log_info("received: %d : %s\n", r->status, r->status_phrase);
    //free_response(r);
}

void
read_file(int socket_fd, char *file)
{
    if (PRINT) log_info("client reading file %s\n", file);
    send_request(socket_fd, READ_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
    if (PRINT) log_info("received: %d : %s : %lu\n", r->status, r->status_phrase, r->body_size);
    // if (r->body_size != 0) if (PRINT) log_info("%s\n", (char*)r->body);
    //free_response(r);
}

void
lock_file(int socket_fd, char *file)
{
    if (PRINT) log_info("client locking file %s\n", file);
    send_request(socket_fd, LOCK_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
    if (PRINT) log_info("received: %d : %s : %lu\n", r->status, r->status_phrase, r->body_size);
    // if (r->body_size != 0) if (PRINT) log_info("%s\n", (char*)r->body);
    //free_response(r);
}

void
unlock_file(int socket_fd, char *file)
{
    if (PRINT) log_info("client unlocking file %s\n", file);
    send_request(socket_fd, UNLOCK_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
    if (PRINT) log_info("received: %d : %s : %lu\n", r->status, r->status_phrase, r->body_size);
    // if (r->body_size != 0) if (PRINT) log_info("%s\n", (char*)r->body);
    //free_response(r);
}

void
remove_file(int socket_fd, char *file)
{
    if (PRINT) log_info("client removing file %s\n", file);
    send_request(socket_fd, REMOVE_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
    if (PRINT) log_info("received: %d : %s : %lu\n", r->status, r->status_phrase, r->body_size);
    if (r->body_size != 0) if (PRINT) log_info("%s\n", (char*)r->body);
    //free_response(r);
}

void 
write_file(int socket_fd, char *file)
{
    if (PRINT) log_info("client writing file %s\n", file);
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
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
    if (PRINT) log_info("received: %d : %s\n", r->status, r->status_phrase);

    if (r->status == FILES_EXPELLED) {
        int n_files = *(int*)r->body;
        if (PRINT) log_info("%d files were expelled\n", n_files);

        while (n_files > 0) {
            response_t *r1 = recv_response(socket_fd);
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
            if (r1->status == INTERNAL_ERROR) break;
            if (PRINT) log_info("received: %d : %s : %lu\n", r1->status, r1->status_phrase, r1->body_size);
            //if (PRINT) log_info("\n%s\n\n%s\n\n", r1->file_path, (char*)r1->body);
            n_files--;
        }
    }
    //free_response(r);
}

void
open_file(int socket_fd, char * file, int flags)
{
    if (PRINT) log_info("client opening file %s\n", file);
    send_request(socket_fd, OPEN_FILE, file, sizeof(int), &flags);
    response_t *r = recv_response(socket_fd);
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
    if (PRINT) log_info("received: %d : %s\n", r->status, r->status_phrase);
    //free_response(r);
}



void
close_file(int socket_fd, char * file)
{
    if (PRINT) log_info("client closing file %s\n", file);
    send_request(socket_fd, CLOSE_FILE, file, 0, NULL);
    response_t *r = recv_response(socket_fd);
if (errno == ENOTCONN || errno == ECONNRESET) exit(-1);
; 
    if (PRINT) log_info("received: %d : %s\n", r->status, r->status_phrase);
    //free_response(r);
}

void
close_connection(int socket_fd)
{
    if (PRINT) log_info("client closing connection\n");
    send_request(socket_fd, CLOSE_CONNECTION, "", 0, NULL);
}

int main(int argc, char const *argv[])
{

#ifdef DEBUG
   log_init(argv[1]);
   set_log_level(LOG_DEBUG);
#else
   log_init(NULL);
#endif

    atexit(cleanup);
    int result;
    struct sockaddr_un serveraddr;
    srand(time(NULL));

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) return -1;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, DEFAULT_SOCKET_PATH);

    result = connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); // Use SUN_LEN
    if (result < 0) return -1;

    if (PRINT) log_info("\n****** START ******\n\n");



    do {

        int flags = 0;

        SET_FLAG(flags, O_CREATE);
        SET_FLAG(flags, O_LOCK);

        open_connection(socket_fd);

        open_file(socket_fd, "file1", flags);
        write_file(socket_fd, "file1");
        close_file(socket_fd, "file1");


        open_file(socket_fd, "file2", flags);
        write_file(socket_fd, "file2");
        unlock_file(socket_fd, "file2");
        read_file(socket_fd, "file2");
        lock_file(socket_fd, "file2");
        remove_file(socket_fd, "file2");
        

        // close_file(socket_fd, "file2");

        

        close_connection(socket_fd);


    } while(0);

    if (PRINT) log_info("\n\n****** END ******\n\n");

    close_log();

    return 0;
}
