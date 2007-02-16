/* $Id$
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
