/*
 * $Id: wcslen.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

size_t wcslen(const wchar_t* str)
{
  const wchar_t* s;

  if (str == 0)
    return 0;
  for (s = str; *s; ++s);
  return s-str;
}

