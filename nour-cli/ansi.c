#include "ansi.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char* ansi_wrap(const char* code, const char* text) {
    u64 len = strlen(code) + strlen(text) + strlen(COLOR_RESET) + 1;
    char* buff = malloc(len);
    if (!buff) return NULL;
    snprintf(buff, len, "%s%s" COLOR_RESET, code, text);
    return buff;
}

static char* fg_black  (const char* t) { return ansi_wrap("\033[30m",   t); }
static char* fg_red    (const char* t) { return ansi_wrap("\033[1;31m", t); }
static char* fg_green  (const char* t) { return ansi_wrap("\033[1;32m", t); }
static char* fg_yellow (const char* t) { return ansi_wrap("\033[1;33m", t); }
static char* fg_blue   (const char* t) { return ansi_wrap("\033[1;34m", t); }
static char* fg_cyan   (const char* t) { return ansi_wrap("\033[1;36m", t); }

static char* bg_red    (const char* t) { return ansi_wrap("\033[41m", t); }
static char* bg_green  (const char* t) { return ansi_wrap("\033[42m", t); }
static char* bg_yellow (const char* t) { return ansi_wrap("\033[43m", t); }
static char* bg_cyan   (const char* t) { return ansi_wrap("\033[46m", t); }


char* bold(const char* t) { return ansi_wrap("\033[1m", t); }
char* dim (const char* t) { return ansi_wrap("\033[2m", t); }


FgColors fg = {
    .black  = fg_black,
    .red    = fg_red,
    .green  = fg_green,
    .yellow = fg_yellow,
    .blue   = fg_blue,
    .cyan   = fg_cyan,
};

BgColors bg = {
    .red    = bg_red,
    .green  = bg_green,
    .yellow = bg_yellow,
    .cyan   = bg_cyan,
};