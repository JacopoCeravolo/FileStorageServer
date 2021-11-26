#include "fileserver_api.h"

int
openFile(const char* pathname, int flags)
{
    printf("> [CLIENT] opening [%s]\n", pathname);

    send_request(socket_fd, OPEN_FILE, pathname, sizeof(int), &flags);

    response_t *response = recv_response(socket_fd);

    printf("> [SERVER] : %s\n", response->status_phrase);
    
    return 0;
}