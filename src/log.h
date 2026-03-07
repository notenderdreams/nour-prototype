#ifndef LOG_H
#define LOG_H

#include <stdio.h>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"

typedef enum {
    LOG_OK,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO
} LogLevel;

void log_print(LogLevel level, const char *format, ...);

#endif // LOG_H
