/*
 * $Id: memcpy.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

void* memcpy(void* to, const void* from, size_t count)
{
  register char *f = (char *)from;
  register char *t = (char *)to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
  return to;
}
