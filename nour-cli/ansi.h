#ifndef ANSI_H
#define ANSI_H

// Escape codes
#define COLOR_RESET     "\033[0m"
#define COLOR_DIM       "\033[2m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_GREEN     "\033[1;32m"
#define COLOR_RED       "\033[1;31m"
#define COLOR_YELLOW    "\033[1;33m"
#define COLOR_BLUE      "\033[1;34m"
#define COLOR_CYAN      "\033[1;36m"
#define COLOR_BLACK     "\033[30m"
#define COLOR_BG_GREEN  "\033[42m"
#define COLOR_BG_RED    "\033[41m"
#define COLOR_BG_YELLOW "\033[43m"
#define COLOR_BG_CYAN   "\033[46m"

// Foreground 
typedef struct {
    char* (*black)  (const char* text);
    char* (*red)    (const char* text);
    char* (*green)  (const char* text);
    char* (*yellow) (const char* text);
    char* (*blue)   (const char* text);
    char* (*cyan)   (const char* text);
} FgColors;

// Background 
typedef struct {
    char* (*red)    (const char* text);
    char* (*green)  (const char* text);
    char* (*yellow) (const char* text);
    char* (*cyan)   (const char* text);
} BgColors;


extern FgColors fg;
extern BgColors bg;


char* bold(const char* text);
char* dim (const char* text);

char* ansi_wrap(const char* code, const char* text);

#endif // ANSI_H