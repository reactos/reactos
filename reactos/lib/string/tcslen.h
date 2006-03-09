/* $Id$
 */

#include <stddef.h>
#include <tchar.h>

size_t _tcslen(const _TCHAR * str)
{
 const _TCHAR * s;

 if(str == 0) return 0;

 for(s = str; *s; ++ s);

 return s - str;
}

/* EOF */
