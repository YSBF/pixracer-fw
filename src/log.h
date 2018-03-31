#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>

int chvsnprintf(char *str, size_t size, const char *fmt, va_list ap);
void log_init(void);

#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
