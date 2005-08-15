// pre-compiled header stuff

#ifndef PCH_H
#define PCH_H

#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // identifier was truncated to '255' characters in the debug information
#endif//_MSC_VER

#include <string>
#include <vector>
#include <map>
#include <set>

#include <stdarg.h>
#ifndef RBUILD
#include <ctype.h>
#include <unistd.h>
#endif

#ifdef _MSC_VER
#define MAX_PATH _MAX_PATH
#endif

#ifndef WIN32
#include <wctype.h>
#include <math.h>

inline char * strlwr(char *x)
{
        char  *y=x;

        while (*y) {
                *y=tolower(*y);
                y++;
        }
        return x;
}

#define _finite __finite
#define _isnan __isnan
#define stricmp strcasecmp
#define MAX_PATH PATH_MAX 
#define _MAX_PATH PATH_MAX
#endif

#endif//PCH_H
