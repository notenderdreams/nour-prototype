#include "log.h"
#include <stdarg.h>

void log_print(LogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    switch (level) {
        case LOG_OK:
            printf("%s[OK]%s ", COLOR_GREEN, COLOR_RESET);
            break;
        case LOG_ERROR:
            fprintf(stderr, "%s[ERROR]%s ", COLOR_RED, COLOR_RESET);
            break;
        case LOG_WARN:
            printf("%s[WARN]%s ", COLOR_YELLOW, COLOR_RESET);
            break;
        case LOG_INFO:
            printf("%s[INFO]%s ", COLOR_BLUE, COLOR_RESET);
            break;
    }
    
    if (level == LOG_ERROR) {
        vfprintf(stderr, format, args);
    } else {
        vprintf(format, args);
    }
    
    va_end(args);
}
