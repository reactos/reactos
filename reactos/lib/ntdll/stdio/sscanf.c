/**/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>
#include <stdio.h>

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

#define debugstr_a(a) (a)
#define TRACE DPRINT
#define WARN DPRINT1

/* vsscanf */
#undef WIDE_SCANF
#undef CONSOLE
#define STRING 1
#include "scanf.h"

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
