#ifndef NOUR_H
#define NOUR_H

typedef struct {
    char *name;
    char *version;
    char **sources;
    char *cc;
    char *build_dir;
    char *output_name;
    char **cflags;
} Project;


#endif // NOUR_H