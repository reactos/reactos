/* $Id: tcsrchr.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include <tchar.h>

_TCHAR * _tcsrchr(const _TCHAR * s, _XINT c)
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
