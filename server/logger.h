#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#include "server_config.h"


typedef enum {
    LOG_FATAL   = 0,
    LOG_ERROR   = 1,
    LOG_INFO    = 2,
    LOG_WARNING = 3,
    LOG_DEBUG   = 4,
} loglevel;

/* Configuration */
void log_init(const char* path);
void set_log_level(loglevel level);

/* Logging Functions */
int logfatal(const char *file, const int line, const char *fmt, ...);
int logerror(const char *file, const int line, const char *fmt, ...);
int loginfo(const char *fmt, ...);
int logwarning(const char *file, const int line, const char *fmt, ...);
int logdebug(const char *file, const int line, const char *fmt, ...); 

/* Cleanup */
void flush_log();
void close_log();

/* Wrappers */
#define log_fatal(fmt, args...) logfatal(__FILE__, __LINE__, fmt, ##args); 
#define log_error(fmt, args...) logerror(__FILE__, __LINE__, fmt, ##args); 
#define log_info(fmt, args...) loginfo(fmt, ##args); 
#define log_warning(fmt, args...) logwarning(__FILE__, __LINE__, fmt, ##args);
#define log_debug(fmt, args...) logdebug(__FILE__, __LINE__, fmt, ##args); 


#endif