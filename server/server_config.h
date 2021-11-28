#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#ifndef  EXTERN
#define  EXTERN  extern
#endif

#include "utils/concurrent_queue.h"
#include "utils/protocol.h"
#include "utils/utilities.h"
#include "utils/logger.h"
#include "server/storage.h"


typedef enum {
    OPEN,
    SHUTDOWN,
    SHUTDOWN_NOW,
} server_mode_t;

typedef struct {
    unsigned int no_of_workers;
    char socket_path[MAX_PATH];
} server_config_t;


EXTERN server_config_t          server_config;
EXTERN server_mode_t            server_status;
EXTERN storage_t                *storage;
EXTERN concurrent_queue_t       *request_queue;
EXTERN volatile long            worker_exit_signal;

#endif