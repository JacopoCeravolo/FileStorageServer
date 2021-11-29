#include "fileserver_api.h"

int openConnection(const char *sockname, int msec, const struct timespec abstime)
{
    /* if ( socket_fd != -1 ) {
        errno = EISCONN;
        return -1;
    } */

    if ( sockname == NULL || msec < 0 ) {
       errno = EINVAL;
       return -1;
    }


    int result;
    struct sockaddr_un serveraddr;

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) return -1;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, sockname);

    struct timespec wait_time;
    wait_time.tv_sec = msec / 1000;
    msec = msec % 1000;
    wait_time.tv_nsec = msec * 1000;

    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    while ( (result = connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
            && current_time.tv_sec < abstime.tv_sec ) {
        
        if( nanosleep(&wait_time, NULL) == -1){
            socket_fd = -1;
            return -1;
        }
        if( clock_gettime(CLOCK_REALTIME, &current_time) == -1){
            socket_fd = -1;
            return -1;        
        }
    }
    if (result < 0) return -1;

    if ( send_request(socket_fd, OPEN_CONNECTION, "", 0, NULL) != 0 ) return -1;

    response_t *handshake = recv_response(socket_fd);
    if (handshake == NULL) return -1;
    
    switch ( handshake->status ) {

        case INTERNAL_ERROR: {
            errno = ECONNABORTED;
            result = -1;
            break;
        }

        case SUCCESS: {
            result = 0;
            break;
        }

        default: {
            errno = EPROTONOSUPPORT;
            result = -1;
            break;
        }
    }

    if (handshake) free_response(handshake);
    return result;
}
