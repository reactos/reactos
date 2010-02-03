/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Misc utils
 */

#ifndef __L2L_UTIL_H__
#define __L2L_UTIL_H__


#define log(outFile, fmt, ...)                          \
    {                                                   \
        fprintf(outFile, fmt, ##__VA_ARGS__);           \
        if (logFile)                                    \
            fprintf(logFile, fmt, ##__VA_ARGS__);       \
    }

#define l2l_dbg(level, ...)                     \
    {                                           \
        if (opt_verbose >= level)               \
            fprintf(stderr, ##__VA_ARGS__);     \
    }

int file_exists(char *name);
int mkPath(char *path, int isDir);
char *basename(char *path);
const char *getFmt(const char *a);
long my_atoi(const char *a);
int isOffset(const char *a);
int copy_file(char *src, char *dst);

#endif /* __L2L_UTIL_H__ */
