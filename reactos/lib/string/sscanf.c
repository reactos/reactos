/**/
#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/umtypes.h>
#include <ndk/rtlfuncs.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/* helper function for *scanf.  Returns the value of character c in the
 * given base, or -1 if the given character is not a digit of the base.
 */
#if 0
static int char2digit(char c, int base) {
    if ((c>='0') && (c<='9') && (c<='0'+base-1)) return (c-'0');
    if (base<=10) return -1;
    if ((c>='A') && (c<='Z') && (c<='A'+base-11)) return (c-'A'+10);
    if ((c>='a') && (c<='z') && (c<='a'+base-11)) return (c-'a'+10);
    return -1;
}
#endif

/* vsscanf */
#undef WIDE_SCANF
#undef CONSOLE
#define STRING 1
//#include "scanf.h"

int sscanf(const char *str, const char *format, ...)
{
    int res = 0;
#if 0
    va_list valist;

    va_start(valist, format);
    res = vsscanf(str, format, valist);
    va_end(valist);
#endif

    return res;
}

/*EOF */
