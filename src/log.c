#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static FILE *log_stream(LogLevel level) {
    return level == LOG_ERROR ? stderr : stdout;
}

static const char *log_label(LogLevel level) {
    switch (level) {
        case LOG_OK:
            return " OK ";
        case LOG_ERROR:
            return "ERR ";
        case LOG_WARN:
            return "WARN";
        case LOG_INFO:
            return "INFO";
        case LOG_ALIGNED:
        default:
            return "    ";
    }
}

static const char *log_badge_style(LogLevel level) {
    switch (level) {
        case LOG_OK:
            return COLOR_BOLD COLOR_BLACK COLOR_BG_GREEN;
        case LOG_ERROR:
            return COLOR_BOLD COLOR_BLACK COLOR_BG_RED;
        case LOG_WARN:
            return COLOR_BOLD COLOR_BLACK COLOR_BG_YELLOW;
        case LOG_INFO:
            return COLOR_BOLD COLOR_BLACK COLOR_BG_CYAN;
        case LOG_ALIGNED:
        default:
            return "";
    }
}

static int log_use_color(FILE *stream) {
    static int initialized = 0;
    static int enabled = 0;

    if (!initialized) {
        enabled = getenv("NO_COLOR") == NULL;
        initialized = 1;
    }

    return enabled && isatty(fileno(stream));
}

static void log_timestamp(char buffer[9]) {
    time_t now = time(NULL);
    struct tm local_now;

    if (localtime_r(&now, &local_now) == NULL) {
        memcpy(buffer, "00:00:00", 9);
        return;
    }

    strftime(buffer, 9, "%H:%M:%S", &local_now);
}

static void log_prefix(FILE *stream, LogLevel level, int use_color) {
    char timestamp[9];
    log_timestamp(timestamp);

    if (use_color) {
        fprintf(stream, "%s%s%s %s[%s]%s ",
                COLOR_DIM,
                timestamp,
                COLOR_RESET,
                log_badge_style(level),
                log_label(level),
                COLOR_RESET);
        return;
    }

    fprintf(stream, "%s [%s] ", timestamp, log_label(level));
}

static void log_aligned_prefix(FILE *stream) {
    fprintf(stream, "%*s", LOG_MESSAGE_OFFSET, "");
}

static void log_message_with_prefix(FILE *stream,
                                    const char *message,
                                    void (*prefix_writer)(FILE *, LogLevel, int),
                                    LogLevel level,
                                    int use_color) {
    const char *cursor = message;

    while (*cursor != '\0') {
        const char *newline = strchr(cursor, '\n');
        if (newline == NULL) {
            fputs(cursor, stream);
            return;
        }

        fwrite(cursor, 1, (size_t)(newline - cursor + 1), stream);
        cursor = newline + 1;
        if (*cursor != '\0') {
            prefix_writer(stream, level, use_color);
        }
    }
}

static void log_plain_prefix(FILE *stream, LogLevel level, int use_color) {
    (void)level;
    (void)use_color;
    log_aligned_prefix(stream);
}

static void log_vprint_prefixed(FILE *stream,
                                void (*prefix_writer)(FILE *, LogLevel, int),
                                LogLevel level,
                                int use_color,
                                const char *format,
                                va_list args) {
    char stack_buffer[1024];
    char *heap_buffer = NULL;
    char *message = stack_buffer;
    va_list args_copy;

    va_copy(args_copy, args);
    int needed = vsnprintf(stack_buffer, sizeof(stack_buffer), format, args);
    if (needed < 0) {
        va_end(args_copy);
        return;
    }

    if ((size_t)needed >= sizeof(stack_buffer)) {
        heap_buffer = malloc((size_t)needed + 1);
        if (heap_buffer != NULL) {
            vsnprintf(heap_buffer, (size_t)needed + 1, format, args_copy);
            message = heap_buffer;
        }
    }

    prefix_writer(stream, level, use_color);
    log_message_with_prefix(stream, message, prefix_writer, level, use_color);
    if (needed == 0 || message[needed - 1] != '\n') {
        fputc('\n', stream);
    }

    fflush(stream);
    free(heap_buffer);
    va_end(args_copy);
}

void log_print(LogLevel level, const char *format, ...) {
    FILE *stream = log_stream(level);
    int use_color = log_use_color(stream);
    va_list args;

    va_start(args, format);
    if (level == LOG_ALIGNED ||
        (level != LOG_OK && level != LOG_ERROR && level != LOG_WARN && level != LOG_INFO)) {
        log_vprint_prefixed(stream, log_plain_prefix, level, 0, format, args);
    } else {
        log_vprint_prefixed(stream, log_prefix, level, use_color, format, args);
    }
    va_end(args);
}
