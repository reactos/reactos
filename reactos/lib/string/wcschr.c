/*
 * $Id: wcschr.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

wchar_t *wcschr(const wchar_t *s, wchar_t c)
{
  wchar_t cc = c;
  while (*s)
  {
    if (*s == cc)
      return (wchar_t *)s;
    s++;
  }
  if (cc == 0)
    return (wchar_t *)s;
  return 0;
}

