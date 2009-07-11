/* $Id$
 */

#include <tchar.h>

_TCHAR * _tcsrchr(const _TCHAR * s, _TCHAR c)
{
 _TCHAR cc = c;
 const _TCHAR * sp = (_TCHAR *)0;

 while(*s)
 {
  if(*s == cc) sp = s;
  s ++;
 }

 if(cc == 0) sp = s;

 return (_TCHAR *)sp;
}

/* EOF */
