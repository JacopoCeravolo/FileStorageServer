#include <string.h>
#include <stdio.h>
#include <stdarg.h> 

#include "logger.h"

void 
log_init(const char* path) 
{
    log_level = LOG_INFO;

    if (path && *path != '\0') {
         strncpy(log_path, path, MAX_PATH);
        fp = fopen(path, "w+");
        if (fp == NULL) // if fopen fails revert to stderr
            fp = stderr;
    }
    else {
        if (fp != NULL && fp != stderr && fp != stdout)
            fclose(fp);

        fp = stderr;
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
    va_list args;
    va_start (args, format);
    fprintf(fp, "%s:%d: [ fatal error ] ", file, line);
    vfprintf (fp, format, args);
    va_end (args);
    fflush(fp);
}

void 
logerror(const char *file, const int line, const char* format, ...) 
{
    if (log_level >= LOG_ERROR) {
        va_list args;
        va_start (args, format);
        fprintf(fp, "%s:%d: [ error ] ", file, line);
        vfprintf (fp, format, args);
        va_end (args);
        fflush(fp);
    }
}

void 
loginfo(const char* format, ...) 
{
    if (log_level >= LOG_INFO) {
        va_list args;
        va_start (args, format);
        vfprintf (fp, format, args);
        va_end (args);
        fflush(fp);
    }
}

void 
logwarning(const char *file, const int line, const char* format, ...) 
{
    if (log_level >= LOG_WARNING) {
        va_list args;
        va_start (args, format);
        fprintf(fp, "%s:%d: [ warning ] ", file, line);
        vfprintf (fp, format, args);
        va_end (args);
        fflush(fp);
    }
}

void 
logdebug(const char *file, const int line, const char* format, ...) 
{
    if (log_level >= LOG_DEBUG) {
        va_list args;
        va_start (args, format);
        fprintf(fp, "%s:%d: ", file, line);
        vfprintf (fp, format, args);
        va_end (args);
        fflush(fp);
    }
}

void 
flush_log() 
{
    fflush(fp);
}

void 
close_log() 
{
    if(fp != stdout)
        fclose(fp);
    fp = NULL;
}