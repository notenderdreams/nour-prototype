#ifndef LOG_H
#define LOG_H

#include "ansi.h"
#include <stdio.h>


#define die(fmt, ...) \
    do { \
        char *_d = fg.red("error:"); \
        fprintf(stderr, "%s " fmt "\n", _d, ##__VA_ARGS__); \
        free(_d); \
        exit(1); \
    } while (0)

#define err(fmt, ...) \
    do { \
        char *_e = fg.red("error:"); \
        fprintf(stderr, "%s " fmt "\n", _e, ##__VA_ARGS__); \
        free(_e); \
    } while (0)


#define warn(fmt, ...) \
    do { \
        char *_w = fg.yellow("warn:"); \
        fprintf(stderr, "%s " fmt "\n", _w, ##__VA_ARGS__); \
        free(_w); \
    } while (0)


#define info(fmt, ...) \
    do { \
        char *_i = fg.cyan("info:"); \
        printf("%s " fmt "\n", _i, ##__VA_ARGS__); \
        free(_i); \
    } while (0)


#define ok(fmt, ...) \
    do { \
        char *_o = fg.green("ok:"); \
        printf("%s " fmt "\n", _o, ##__VA_ARGS__); \
        free(_o); \
    } while (0)


#define status(label, fmt, ...) \
    do { \
        char *_l = fg.green(label); \
        printf("  %-12s " fmt "\n", _l, ##__VA_ARGS__); \
        free(_l); \
    } while (0)

#define status_info(label, fmt, ...) \
    do { \
        char *_l = fg.cyan(label); \
        printf("  %-12s " fmt "\n", _l, ##__VA_ARGS__); \
        free(_l); \
    } while (0)

#endif /* LOG_H */