/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Misc utils
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "compat.h"
#include "util.h"
#include "options.h"

int
set_LogFile(FILE *logFile)
{
    if (*opt_logFile)
    {
        if (logFile)
            fclose(logFile);
        logFile = NULL;

        if (strcmp(opt_logFile,"none") == 0)
            return 0; //just close

        logFile = fopen(opt_logFile, "a");
        if (logFile)
        {
            // disable buffering so fflush is not needed
            if (!opt_buffered)
            {
                l2l_dbg(1, "Disabling log buffering on %s\n", opt_logFile);
                setbuf(logFile, NULL);
            }
            else
                l2l_dbg(1, "Enabling log buffering on %s\n", opt_logFile);
        }
        else
        {
            l2l_dbg(0, "Could not open logfile %s (%s)\n", opt_logFile, strerror(errno));
            return 2;
        }
    }
    return 0;
}

int
file_exists(char *name)
{
    FILE *f;

    f = fopen(name, "r");
    if (!f)
        return 0;
    fclose(f);
    return 1;
}

/* Do this in reverse (recursively)
   This saves many system calls if the path is likely
   to already exist (creating large trees).
*/
int
mkPath(char *path, int isDir)
{
    char *s;
    int res = 0;

    if (isDir)
    {
        res = MKDIR(path);
        if (!res || (res == -1 && errno == EEXIST))
            return 0;
    }
    // create parent dir
    if ((s = strrchr(path, PATH_CHAR)))
    {
        *s = '\0';
        res = mkPath(path, 1);
        *s = PATH_CHAR;
    }

    if (!res && isDir)
        res = MKDIR(path);

    return res;
}

#if 0
static FILE *
rfopen(char *path, char *mode)
{
    FILE *f = NULL;
    char tmppath[MAX_PATH]; // Don't modify const strings

    strcpy(tmppath, path);
    f = fopen(tmppath, mode);
    if (!f && !mkPath(tmppath, 0))
        f = fopen(tmppath, mode);
    return f;
}
#endif


char *
basename(char *path)
{
    char *base;

    base = strrchr(path, PATH_CHAR);
    if (base)
        return ++base;
    return path;
}

const char *
getFmt(const char *a)
{
    const char *fmt = "%x";

    if (*a == '0')
    {
        switch (*++a)
        {
        case 'x':
            fmt = "%x";
            ++a;
            break;
        case 'd':
            fmt = "%d";
            ++a;
            break;
        default:
            fmt = "%o";
            break;
        }
    }
    return fmt;
}

long
my_atoi(const char *a)
{
    int i = 0;
    sscanf(a, getFmt(a), &i);
    return i;
}

int
isOffset(const char *a)
{
    int i = 0;
    if (strchr(a, '.'))
        return 0;
    return sscanf(a, getFmt(a), &i);
}

int
copy_file(char *src, char *dst)
{
    char Line[LINESIZE];

    sprintf(Line, CP_FMT, src, dst);
    l2l_dbg(2, "Executing: %s\n", Line);
    remove(dst);
    if (file_exists(dst))
    {
        l2l_dbg(0, "Cannot remove dst %s before copy\n", dst);
        return 1;
    }
    if (system(Line) < 0)
    {
        l2l_dbg(0, "Cannot copy %s to %s\n", src, dst);
        l2l_dbg(1, "Failed to execute: '%s'\n", Line);
        return 2;
    }

    if (!file_exists(dst))
    {
        l2l_dbg(0, "Dst %s does not exist after copy\n", dst);
        return 2;
    }
    return 0;
}
