#include "fileserver_api.h"

#include <unistd.h>

int 
closeConnection(const char* sockname)
{
    int result;
    
    send_request(socket_fd, CLOSE_CONNECTION, "", 0, NULL);
    
    close(socket_fd);

    return 0;
}