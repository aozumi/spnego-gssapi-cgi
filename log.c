#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "log.h"

void
gsscgi_log(const char *level, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    gsscgi_vlog(level, format, ap);
    va_end(ap);
}

void
gsscgi_vlog(const char *level, const char *format, va_list ap)
{
    fprintf(stderr, "%s: ", level);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
}

void
gsscgi_error(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    gsscgi_vlog("ERROR", format, ap);
    va_end(ap);
}

void
gsscgi_perror(const char *msg)
{
    gsscgi_error("%s: %s", msg, strerror(errno));
}

void
gsscgi_debug(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    gsscgi_vlog("DEBUG", format, ap);
    va_end(ap);
}

void
gsscgi_info(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    gsscgi_vlog("INFO", format, ap);
    va_end(ap);
}
