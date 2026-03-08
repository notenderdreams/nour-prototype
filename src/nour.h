#ifndef NOUR_H
#define NOUR_H

typedef struct {
    char *version;
    char **sources;
    char *cc;
    char *build_dir;
    char **cflags;
} Project;


#endif // NOUR_H
