/*
 * $Id: wcsrchr.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

wchar_t *
wcsrchr(const wchar_t *s, wchar_t c)
{
  wchar_t cc = c;
  const wchar_t *sp=(wchar_t *)0;
  while (*s)
  {
    if (*s == cc)
      sp = s;
    s++;
  }
  if (cc == 0)
    sp = s;
  return (wchar_t *)sp;
}

