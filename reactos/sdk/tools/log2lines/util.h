/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Misc utils
 */

#pragma once

#include <stdio.h>

#include "cmd.h"
#include "options.h"

#define log(outFile, fmt, ...)                          \
    {                                                   \
        fprintf(outFile, fmt, ##__VA_ARGS__);           \
        if (logFile)                                    \
            fprintf(logFile, fmt, ##__VA_ARGS__);       \
    }

#define esclog(outFile, fmt, ...)                           \
    {                                                       \
        log(outFile, KDBG_ESC_RESP fmt, ##__VA_ARGS__);     \
    }

#define clilog(outFile, fmt, ...)                       \
    {                                                   \
        if (opt_cli)                                    \
            esclog(outFile, fmt, ##__VA_ARGS__)         \
        else                                            \
            log(outFile, fmt, ##__VA_ARGS__);           \
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
int set_LogFile(FILE **plogFile);

/* EOF */
