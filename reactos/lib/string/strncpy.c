/*
 * $Id: strncpy.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

char *
strncpy(char *dst, const char *src, size_t n)
{
  if (n != 0) {
    char *d = dst;
    const char *s = src;

    do {
      if ((*d++ = *s++) == 0)
      {
	while (--n != 0)
	  *d++ = 0;
	break;
      }
    } while (--n != 0);
  }
  return dst;
}
