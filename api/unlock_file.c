#include "fileserver_api.h"

int 
unlockFile(const char* pathname)
{
    printf("> [CLIENT] unlocking [%s]\n", pathname);

    send_request(socket_fd, UNLOCK_FILE, pathname, 0, NULL);

    response_t *response = recv_response(socket_fd);

    printf("> [SERVER] : %s\n", response->status_phrase);
    return 0;
}