/* $Id: tcsncmp.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include <stddef.h>
#include <tchar.h>

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
