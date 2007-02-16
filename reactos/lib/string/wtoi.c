#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * @implemented
 * copy _i64toa from wine cvs 2006 month 05 day 21
 */
int
_wtoi(const wchar_t *str)
{
   return _wtol(str);
}
