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

#ifdef _MSC_VER
#define MAX_PATH _MAX_PATH
#endif

#ifndef WIN32
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <math.h>

inline char* strlwr ( char* str )
{
  char* p = str;
  while ( *p )
    *p++ = tolower(*p);
  return str;
}

inline char* strupr ( char* str )
{
  char *c = str;
  while ( *str++ )
    toupper( *str );
  return c;
}

#define _finite __finite
#define _isnan __isnan
#define stricmp strcasecmp
#define MAX_PATH PATH_MAX 
#define _MAX_PATH PATH_MAX
#endif

#endif//PCH_H
