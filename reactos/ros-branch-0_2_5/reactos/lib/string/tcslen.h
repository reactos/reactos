/* $Id: tcslen.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
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
