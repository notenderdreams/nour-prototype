#ifndef LOG_H
#define LOG_H

#include <stdio.h>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_DIM     "\033[2m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_BLACK   "\033[30m"

#define COLOR_BG_GREEN  "\033[42m"
#define COLOR_BG_RED    "\033[41m"
#define COLOR_BG_YELLOW "\033[43m"
#define COLOR_BG_CYAN   "\033[46m"

#define LOG_MESSAGE_OFFSET 16

typedef enum {
    LOG_ALIGNED,
    LOG_OK,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO
} LogLevel;

void log_print(LogLevel level, const char *format, ...);

#endif // LOG_H
