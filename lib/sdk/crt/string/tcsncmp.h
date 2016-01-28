
#include <stddef.h>
#include <tchar.h>

#if defined(_MSC_VER) && defined(_M_ARM)
#pragma function(_tcsncmp)
#endif /* _MSC_VER */

int _tcsncmp(const _TCHAR * s1, const _TCHAR * s2, size_t n)
{
 if(n == 0) return 0;

 do
 {
  if(*s1 != *s2 ++) return *s1 - *-- s2;
  if(*s1 ++ == 0) break;
 }
 while (-- n != 0);

 return 0;
}

/* EOF */
