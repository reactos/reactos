/*
 * $Id: wcsnlen.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

int wcsnlen(const wchar_t *str, size_t count)
{
  const wchar_t *s;

  if (str == 0)
    return 0;
  for (s = str; *s && count; ++s, count--);
  return s-str;
}

