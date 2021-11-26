#include "fileserver_api.h"

int openConnection(const char *sockname, int msec, const struct timespec abstime)
{
    /* Initialize sockaddr_un and tries to connect */

    if (sockname == NULL || msec < 0) {
       errno = EINVAL;
       return -1;
    }

    printf("> [CLIENT] opening connection\n");

    int result;
    struct sockaddr_un serveraddr;

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) return -1;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, sockname);

    result = connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); // Use SUN_LEN
    if (result < 0) return -1;

    send_request(socket_fd, OPEN_CONNECTION, "", 0, NULL);

    response_t *handshake;
    handshake = recv_response(socket_fd);

    printf("> [SERVER] : %s\n", handshake->status_phrase);

    return 0;

}