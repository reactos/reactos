/*
 * $Id: strncat.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

char *
strncat(char *dst, const char *src, size_t n)
{
  if (n != 0)
  {
    char *d = dst;
    const char *s = src;

    while (*d != 0)
      d++;
    do {
      if ((*d = *s++) == 0)
	break;
      d++;
    } while (--n != 0);
    *d = 0;
  }
  return dst;
}
