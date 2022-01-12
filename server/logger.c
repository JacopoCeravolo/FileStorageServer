#include <string.h>
#include <stdio.h>
#include <stdarg.h> 
#include <time.h>
#include <sys/timeb.h>

#include "logger.h"
#include "utils/utilities.h"

static loglevel log_level;

static char* 
get_timestamp(char* buf) {
    int bytes;
    struct timeb start;
    ftime(&start);
    bytes = strftime(buf, 20, "%H:%M:%S", localtime(&start.time));
    sprintf(&buf[bytes], ".%03u", start.millitm);
    return buf;
}

void 
log_init(const char* path) 
{
    log_level = LOG_INFO;

    if (path && *path != '\0') {
        log_file = fopen(path, "w+");
        if (log_file == NULL) // if fopen fails revert to stderr
            log_file = stderr;
    }
    else {
        if (log_file != NULL && log_file != stderr && log_file != stdout)
            fclose(log_file);

        log_file = stderr;
    }
}

void 
set_log_level(loglevel level) 
{
    log_level = level;
}

void 
logfatal(const char *file, const int line, const char* format, ...) 
{
    char tmp[50] = { 0 };
    va_list args;
    va_start (args, format);

    lock_return((&log_file_mtx), NULL);
    fprintf(log_file, "%s ", get_timestamp(tmp));
    fprintf(log_file, "%s:%d: [ FATAL ERROR ] ", file, line);
    vfprintf (log_file, format, args);
    unlock_return((&log_file_mtx), NULL);

    va_end (args);
    fflush(log_file);
}

void 
logerror(const char *file, const int line, const char* format, ...) 
{
    if (log_level >= LOG_ERROR) {
        char tmp[50] = { 0 };
        va_list args;
        va_start (args, format);

        lock_return((&log_file_mtx), NULL);
        fprintf(log_file, "%s ", get_timestamp(tmp));
        fprintf(log_file, "[ ERROR ] ");
        vfprintf (log_file, format, args);
        unlock_return((&log_file_mtx), NULL);

        va_end (args);
        fflush(log_file);
    }
}

void 
loginfo(const char* format, ...) 
{
    if (log_level >= LOG_INFO) {
        char tmp[50] = { 0 };
        va_list args;
        va_start (args, format);

        lock_return((&log_file_mtx), NULL);
        fprintf(log_file, "%s ", get_timestamp(tmp));
        fprintf(log_file, "[ INFO ] ");
        vfprintf (log_file, format, args);
        unlock_return((&log_file_mtx), NULL);

        va_end (args);
        fflush(log_file);
    }
}

void 
logwarning(const char *file, const int line, const char* format, ...) 
{
    if (log_level >= LOG_WARNING) {
        char tmp[50] = { 0 };
        va_list args;
        va_start (args, format);

        lock_return((&log_file_mtx), NULL);
        fprintf(log_file, "%s ", get_timestamp(tmp));
        fprintf(log_file, "%s:%d: [ WARNING ] ", file, line);
        vfprintf (log_file, format, args);
        unlock_return((&log_file_mtx), NULL);
        
        va_end (args);
        fflush(log_file);
    }
}

void 
logdebug(const char *file, const int line, const char* format, ...) 
{
    if (log_level >= LOG_DEBUG) {
        char tmp[50] = { 0 };
        va_list args;
        va_start (args, format);

        lock_return((&log_file_mtx), NULL);
        fprintf(log_file, "%s ", get_timestamp(tmp));
        fprintf(log_file, "%s:%d: ", file, line);
        vfprintf (log_file, format, args);
        unlock_return((&log_file_mtx), NULL);
        
        va_end (args);
        fflush(log_file);
    }
}

void 
flush_log() 
{
    fflush(log_file);
}

void 
close_log() 
{
    if(log_file != stdout)
        fclose(log_file);
    log_file = NULL;
}
