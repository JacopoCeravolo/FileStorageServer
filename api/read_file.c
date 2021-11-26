#include "fileserver_api.h"

int 
readFile(const char* pathname, void** buf, size_t* size)
{
    printf("> [CLIENT] reading [%s]\n", pathname);

    send_request(socket_fd, READ_FILE, pathname, 0, NULL);

    response_t *response = recv_response(socket_fd);

    printf("> [SERVER] : %s\n", response->status_phrase);
    if (response->status == SUCCESS) {
        printf("*** Received ***\n");
        printf("%s\n", (char*)response->body);
    }
    return 0;
}