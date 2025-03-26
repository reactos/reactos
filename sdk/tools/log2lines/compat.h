#pragma once

#if defined(_WIN32)

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#include <direct.h>

#define POPEN           _popen
#define PCLOSE          _pclose
#define MKDIR(d)        _mkdir(d)
#define DEV_NULL        "NUL"
#define DOS_PATHS
#define PATH_CHAR       '\\'
#define PATH_STR        "\\"
#define PATHCMP         strcasecmp
#define CP_CMD          "copy /Y "
#define DIR_FMT         "dir /a:-d /s /b %s > %s"

#else /* not defined (_WIN32) */
#include <sys/stat.h>

#define POPEN           popen
#define PCLOSE          pclose
#define MKDIR(d)        mkdir(d, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)
#define DEV_NULL        "/dev/null"
#define UNIX_PATHS
#define PATH_CHAR       '/'
#define PATH_STR        "/"
#define PATHCMP         strcasecmp
#define CP_CMD          "cp -f "
#define DIR_FMT         "find %s -type f > %s"

#endif /* not defined (_WIN32) */

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

/* EOF */
