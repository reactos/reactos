/* $Id$
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
