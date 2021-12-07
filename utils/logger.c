#include <string.h>
#include <stdio.h>
#include <stdarg.h> 
#include <time.h>
#include <sys/timeb.h>

#include "logger.h"

static loglevel log_level;
static char log_path[MAX_PATH];
static FILE* fp;

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
    char tmp[50] = { 0 };
    va_list args;
    va_start (args, format);
    fprintf(fp, "%s ", get_timestamp(tmp));
    fprintf(fp, "%s:%d: [ fatal error ] ", file, line);
    vfprintf (fp, format, args);
    va_end (args);
    fflush(fp);
}

void 
logerror(const char *file, const int line, const char* format, ...) 
{
    if (log_level >= LOG_ERROR) {
        char tmp[50] = { 0 };
        va_list args;
        va_start (args, format);
        fprintf(fp, "%s ", get_timestamp(tmp));
        fprintf(fp, "[ error ] ");
        vfprintf (fp, format, args);
        va_end (args);
        fflush(fp);
    }
}

void 
loginfo(const char* format, ...) 
{
    if (log_level >= LOG_INFO) {
        char tmp[50] = { 0 };
        va_list args;
        va_start (args, format);
        fprintf(fp, "%s ", get_timestamp(tmp));
        vfprintf (fp, format, args);
        va_end (args);
        fflush(fp);
    }
}

void 
logwarning(const char *file, const int line, const char* format, ...) 
{
    if (log_level >= LOG_WARNING) {
        char tmp[50] = { 0 };
        va_list args;
        va_start (args, format);
        fprintf(fp, "%s ", get_timestamp(tmp));
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
        char tmp[50] = { 0 };
        va_list args;
        va_start (args, format);
        fprintf(fp, "%s ", get_timestamp(tmp));
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
