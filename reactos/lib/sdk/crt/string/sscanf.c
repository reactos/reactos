#include <precomp.h>

#include <wchar.h>
#include <ctype.h>

#define NDEBUG
#include <internal/debug.h>

#define WARN DPRINT1


#define EOF		(-1)

/* helper function for *scanf.  Returns the value of character c in the
 * given base, or -1 if the given character is not a digit of the base.
 */
static int char2digit(char c, int base) {
    if ((c>='0') && (c<='9') && (c<='0'+base-1)) return (c-'0');
    if (base<=10) return -1;
    if ((c>='A') && (c<='Z') && (c<='A'+base-11)) return (c-'A'+10);
    if ((c>='a') && (c<='z') && (c<='a'+base-11)) return (c-'a'+10);
    return -1;
}

/* vsscanf */
#undef WIDE_SCANF
#undef CONSOLE
#define STRING 1
#include "wine/scanf.h"

int sscanf(const char *str, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vsscanf(str, format, valist);
    va_end(valist);
    return res;
}

/*EOF */
