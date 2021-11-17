#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include "utilities.h"

typedef enum {
    LOG_FATAL   = 0,
    LOG_ERROR   = 1,
    LOG_INFO    = 2,
    LOG_WARNING = 3,
    LOG_DEBUG   = 4,
} loglevel;

static loglevel log_level;
static char log_path[MAX_PATH];
static FILE* fp;

/* Configuration */
void log_init(const char* path);
void set_log_level(loglevel level);

/* Logging Functions */
void logfatal(const char *file, const int line, const char *fmt, ...);
void logerror(const char *file, const int line, const char *fmt, ...);
void loginfo(const char *fmt, ...);
void logwarning(const char *file, const int line, const char *fmt, ...);
void logdebug(const char *file, const int line, const char *fmt, ...); 

/* Cleanup */
void flush_log();
void close_log();

#define log_fatal(fmt, args...) logfatal(__FILE__, __LINE__, fmt, ##args); flush_log();
#define log_error(fmt, args...) logerror(__FILE__, __LINE__, fmt, ##args); flush_log();
#define log_info(fmt, args...) loginfo(fmt, ##args); flush_log();
#define log_warning(fmt, args...) logwarning(__FILE__, __LINE__, fmt, ##args); flush_log();
#define log_debug(fmt, args...) logdebug(__FILE__, __LINE__, fmt, ##args); flush_log();


#endif