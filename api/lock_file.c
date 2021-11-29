#include "fileserver_api.h"

int 
lockFile(const char* pathname)
{
    printf("> [CLIENT] locking [%s]\n", pathname);

    send_request(socket_fd, LOCK_FILE, pathname, 0, NULL);

    response_t *response = recv_response(socket_fd);

    printf("> [SERVER] : %s\n", response->status_phrase);
    return 0;
}
