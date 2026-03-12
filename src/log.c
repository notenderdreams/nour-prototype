#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static FILE *g_log_file = NULL;
static int g_log_is_temp = 0;

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

int console_use_color(void) {
    static int initialized = 0;
    static int enabled     = 0;
    if (!initialized) {
        enabled     = getenv("NO_COLOR") == NULL && isatty(STDOUT_FILENO);
        initialized = 1;
    }
    return enabled;
}

void console_out(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fflush(stdout);
}

void log_file_init(const char *path) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    g_log_file = fopen(path, "w");
    g_log_is_temp = 0;
    if (!g_log_file)
        fprintf(stderr, "warning: could not open log file: %s\n", path);
}

void log_file_init_temp(void) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    g_log_file = tmpfile();
    g_log_is_temp = (g_log_file != NULL);
    if (!g_log_file)
        fprintf(stderr, "warning: could not create temporary log file\n");
}

void log_file_flush_to(const char *path) {
    if (!g_log_file || !g_log_is_temp) {
        log_file_init(path);
        return;
    }

    fflush(g_log_file);
    rewind(g_log_file);

    FILE *dest = fopen(path, "w");
    if (!dest) {
        fprintf(stderr, "warning: could not open log file: %s\n", path);
        fclose(g_log_file);
        g_log_file = NULL;
        g_log_is_temp = 0;
        return;
    }

    char buffer[4096];
    size_t read_count;
    while ((read_count = fread(buffer, 1, sizeof(buffer), g_log_file)) > 0)
        fwrite(buffer, 1, read_count, dest);

    fclose(g_log_file);
    g_log_file = dest;
    g_log_is_temp = 0;
}

void log_file_close(void) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    g_log_is_temp = 0;
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

// ── File-output prefix writers ────────────────────────────────────────

static const char *log_file_label(LogLevel level) {
    switch (level) {
        case LOG_OK:    return " OK ";
        case LOG_ERROR: return "ERR ";
        case LOG_WARN:  return "WARN";
        case LOG_INFO:  return "INFO";
        default:        return "    ";
    }
}

static void log_file_prefix_w(FILE *stream, LogLevel level, int use_color) {
    (void)use_color;
    char timestamp[9];
    log_timestamp(timestamp);
    fprintf(stream, "[%s] [%s] ", timestamp, log_file_label(level));
}

static void log_file_plain_w(FILE *stream, LogLevel level, int use_color) {
    (void)level;
    (void)use_color;
    char timestamp[9];
    log_timestamp(timestamp);
    fprintf(stream, "[%s]        ", timestamp);
}

// Emit a pre-formatted message to a stream with the given prefix writer.
static void log_emit(FILE *stream, LogLevel level, int use_color,
                     void (*prefix_w)(FILE *, LogLevel, int),
                     const char *message, int needed) {
    prefix_w(stream, level, use_color);
    log_message_with_prefix(stream, message, prefix_w, level, use_color);
    if (needed == 0 || message[needed - 1] != '\n')
        fputc('\n', stream);
    fflush(stream);
}

void log_print(LogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);

    // Format message into a buffer once.
    char  stack_buf[1024];
    char *heap_buf = NULL;
    char *message  = stack_buf;
    va_list copy;
    va_copy(copy, args);
    int needed = vsnprintf(stack_buf, sizeof(stack_buf), format, args);
    va_end(args);
    if (needed < 0) { va_end(copy); return; }
    if ((size_t)needed >= sizeof(stack_buf)) {
        heap_buf = malloc((size_t)needed + 1);
        if (heap_buf) {
            vsnprintf(heap_buf, (size_t)needed + 1, format, copy);
            message = heap_buf;
        }
    }
    va_end(copy);

    int is_aligned = (level == LOG_ALIGNED ||
                      (level != LOG_OK && level != LOG_ERROR &&
                       level != LOG_WARN && level != LOG_INFO));

    if (g_log_file != NULL) {
        // Verbose output to log file.
        void (*fpfx)(FILE *, LogLevel, int) =
            is_aligned ? log_file_plain_w : log_file_prefix_w;
        log_emit(g_log_file, level, 0, fpfx, message, needed);
        // Errors and warnings are also echoed to the terminal.
        if (level == LOG_ERROR || level == LOG_WARN) {
            int use_color = log_use_color(stderr);
            log_emit(stderr, level, use_color, log_prefix, message, needed);
        }
    } else {
        // No log file: use original console output path.
        FILE *stream = log_stream(level);
        int use_color = log_use_color(stream);
        void (*tpfx)(FILE *, LogLevel, int) =
            is_aligned ? log_plain_prefix : log_prefix;
        log_emit(stream, level, use_color, tpfx, message, needed);
    }

    free(heap_buf);
}
