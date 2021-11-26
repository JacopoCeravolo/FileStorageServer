#include "fileserver_api.h"

int 
closeFile(const char* pathname)
{
    printf("> [CLIENT] closing [%s]\n", pathname);

    send_request(socket_fd, CLOSE_FILE, pathname, 0, NULL);

    response_t *response = recv_response(socket_fd);

    printf("> [SERVER] : %s\n", response->status_phrase);
    
    return 0;
}