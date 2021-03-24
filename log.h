#ifndef LOG_H_61BA0A25_02D5_468C_8759_218FB2B38295
#define LOG_H_61BA0A25_02D5_468C_8759_218FB2B38295

#include <stdarg.h>

void gsscgi_log(const char *level, const char *format, ...);
void gsscgi_vlog(const char *level, const char *format, va_list ap);
void gsscgi_error(const char *format, ...);
void gsscgi_perror(const char *msg);
void gsscgi_debug(const char *format, ...);
void gsscgi_info(const char *format, ...);


#endif // LOG_H_61BA0A25_02D5_468C_8759_218FB2B38295
