
#include <stddef.h>
#include <tchar.h>

#if defined(_MSC_VER) && _MSC_VER >= 1932 // VS2022 version 17.2 and later
#pragma function(_tcsncpy)
#endif /* _MSC_VER */

_TCHAR * _tcsncpy(_TCHAR * dst, const _TCHAR * src, size_t n)
{
 if(n != 0)
 {
  _TCHAR * d = dst;
  const _TCHAR * s = src;

  do
  {
   if((*d ++ = *s ++) == 0)
   {
    while (-- n != 0) *d ++ = 0;
    break;
   }
  }
  while(-- n != 0);
 }

 return dst;
}

/* EOF */
