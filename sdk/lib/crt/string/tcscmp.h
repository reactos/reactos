
#include <tchar.h>

#if defined(_MSC_VER)
#pragma function(_tcscmp)
#endif /* _MSC_VER */

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
