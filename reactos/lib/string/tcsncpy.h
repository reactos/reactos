/* $Id: tcsncpy.h,v 1.2 2004/01/27 21:43:47 gvg Exp $
 */

#include <stddef.h>
#include <tchar.h>

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
    break;
   }
  }
  while(-- n != 0);
 }

 return dst;
}

/* EOF */
