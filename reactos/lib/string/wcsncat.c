/*
 * $Id: wcsncat.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

wchar_t *
wcsncat(wchar_t *dst, const wchar_t *src, size_t n)
{
  if (n != 0)
  {
    wchar_t *d = dst;
    const wchar_t *s = src;

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
