/* $Id: tcscmp.h 30283 2007-11-08 21:06:20Z fireball $
 */

#include <tchar.h>

int _tcscmp(const _TCHAR* s1, const _TCHAR* s2)
{
 while(*s1 == *s2)
 {
  if(*s1 == 0) return 0;

  s1 ++;
  s2 ++;
 }

 return *s1 - *s2;
}

/* EOF */
