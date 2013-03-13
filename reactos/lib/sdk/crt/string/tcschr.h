/* $Id: tcschr.h 30283 2007-11-08 21:06:20Z fireball $
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
