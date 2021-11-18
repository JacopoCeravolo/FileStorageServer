#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#ifndef  EXTERN
#define  EXTERN  extern
#endif

#include "server/storage.h"
#include "utils/hash_map.h"

EXTERN storage_t *storage;
EXTERN int       exit_signal;

#endif