#include "fileserver_api.h"

int 
removeFile(const char* pathname)
{
    printf("> [CLIENT] removing [%s]\n", pathname);

    send_request(socket_fd, REMOVE_FILE, pathname, 0, NULL);

    response_t *response = recv_response(socket_fd);

    printf("> [SERVER] : %s\n", response->status_phrase);
    return 0;
}