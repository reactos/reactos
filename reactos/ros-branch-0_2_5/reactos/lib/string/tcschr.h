/* $Id: tcschr.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include <tchar.h>

_TCHAR * _tcschr(const _TCHAR * s, _XINT c)
{
 _TCHAR cc = c;

 while(*s)
 {
  if(*s == cc) return (_TCHAR *)s;

  s++;
 }

 if(cc == 0) return (_TCHAR *)s;

 return 0;
}

/* EOF */
