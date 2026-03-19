#include "fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <ftw.h>
#include <unistd.h>


static i32 _rm_entry(const char *fpath, const struct stat *sb,
                     i32 typeflag, struct FTW *ftwbuf) {
    (void)sb; (void)ftwbuf;
    if (typeflag == FTW_DP || typeflag == FTW_D)
        return rmdir(fpath);
    return unlink(fpath);
}


/* Return codes:
 *  0  -> success / path is a directory
 *  1  -> path exists but is not a directory
 *  2  -> path does not exist
 * -1  -> invalid args or system error
 *        (permissions, bad path, etc.)
 */
i32 check_dir_exists(const char *path) {
    if (!path)
        return -1;

    struct stat st;
    if (stat(path, &st) != 0) {
        if (errno == ENOENT)
            return 2;
        return -1;
    }

    return !S_ISDIR(st.st_mode);
}

/* Return codes:
 *  0  -> ok
 * -1  -> invalid args or strdup failed
 * -2  -> mkdir failed (errno is set)
 */
i32 create_dir(const char *path) {
    if (!path)
        return -1;

    char *temp = strdup(path);
    if (!temp)
        return -1;

    u64 len = strlen(temp);

    if (len > 1 && temp[len - 1] == '/')
        temp[len - 1] = '\0';

    for (char *p = temp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';

            if (mkdir(temp, 0755) != 0 && errno != EEXIST) {
                free(temp);
                return -2;
            }

            *p = '/';
        }
    }

    if (mkdir(temp, 0755) != 0 && errno != EEXIST) {
        free(temp);
        return -2;
    }

    free(temp);
    return 0;
}

/* Return codes:
 *  0  -> ok
 * -1  -> invalid args
 * -2  -> removal failed (errno is set)
 */
i32 remove_dir(const char *path) {
    if (!path)
        return -1;

    if (nftw(path, _rm_entry, 64, FTW_DEPTH | FTW_PHYS)) 
        return -2;
    
    return 0;
}

/* Return codes:
 *  0  -> ok
 * -1  -> invalid args
 * -2  -> fopen failed
 * -3  -> write failed
 */
i32 create_file(const char *content, const char *file_path) {
    if (!file_path || !content)
        return -1;

    FILE *f = fopen(file_path, "w");
    if (!f)
        return -2;

    if (fputs(content, f) == EOF) {
        fclose(f);
        return -3;
    }

    fclose(f);
    return 0;
}